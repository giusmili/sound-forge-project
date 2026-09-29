const ELEMENT_IDS = [
  'fileInput', 'addEmptyTrackBtn', 'exportBtn',
  'playBtn', 'pauseBtn', 'stopBtn', 'rewindBtn', 'loopBtn', 'metronomeBtn',
  'masterVolume', 'bpmInput', 'zoomRange', 'timeDisplay',
  'tracksContainer', 'trackTemplate', 'ruler', 'dropZone',
  'trackCount', 'projectDuration', 'statusText'
];

const els = Object.fromEntries(ELEMENT_IDS.map(id => [id, document.getElementById(id)]));

let audioCtx = null;
let masterGain = null;
let tracks = [];
let nextTrackId = 1;
let playing = false;
let pausedAt = 0;
let startedAt = 0;
let rafId = null;
let loopEnabled = false;
let metronomeEnabled = false;
let metronomeTimer = null;
let metronomeNextBeat = 0;
let secondsPer100px = 5;
let resizeTimer = null;

const state = {
  projectDuration: 30,
  lastKnownPlayhead: 0
};

function ensureAudio() {
  if (!audioCtx) {
    const AudioContextCtor = window.AudioContext || window['webkitAudioContext'];
    audioCtx = new AudioContextCtor();
    masterGain = audioCtx.createGain();
    masterGain.gain.value = Number(els.masterVolume.value);
    masterGain.connect(audioCtx.destination);
  }
  if (audioCtx.state === 'suspended') audioCtx.resume();
  return audioCtx;
}

function setStatus(text) {
  els.statusText.textContent = text;
}

function formatTime(sec, precise = true) {
  sec = Math.max(0, Number.isFinite(sec) ? sec : 0);
  const minutes = Math.floor(sec / 60);
  const seconds = Math.floor(sec % 60);
  const ms = Math.floor((sec % 1) * 1000);
  return `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}${precise ? '.' + String(ms).padStart(3, '0') : ''}`;
}

function getProjectDuration() {
  const maxTrack = tracks.reduce((max, t) => {
    const end = (t.offset ?? 0) + (t.buffer?.duration ?? 0);
    return Math.max(max, end);
  }, 0);
  return Math.max(30, maxTrack + 2);
}

function getPixelsPerSecond() {
  return 100 / secondsPer100px;
}

function updateProjectInfo() {
  state.projectDuration = getProjectDuration();
  els.trackCount.textContent = String(tracks.length);
  els.projectDuration.textContent = formatTime(state.projectDuration, false);
  renderRuler();
  renderAllWaveforms();
  updatePlayhead(state.lastKnownPlayhead);
}

function renderRuler() {
  const pps = getPixelsPerSecond();
  const width = Math.max(720, Math.ceil(state.projectDuration * pps));
  els.ruler.style.width = `${width}px`;
  els.ruler.innerHTML = '';

  const minorStep = secondsPer100px <= 3 ? 1 : 5;
  const majorStep = secondsPer100px <= 3 ? 5 : 10;
  const fragment = document.createDocumentFragment();

  for (let s = 0; s <= state.projectDuration; s += minorStep) {
    const x = s * pps;
    const tick = document.createElement('div');
    tick.className = s % majorStep === 0 ? 'tick major' : 'tick';
    tick.style.left = `${x}px`;
    fragment.appendChild(tick);

    if (s % majorStep === 0) {
      const label = document.createElement('div');
      label.className = 'tick-label';
      label.style.left = `${x}px`;
      label.textContent = `${Math.floor(s / 60)}:${String(Math.floor(s % 60)).padStart(2, '0')}`;
      fragment.appendChild(label);
    }
  }
  els.ruler.appendChild(fragment);
}

function createTrack(name = `Piste ${nextTrackId}`) {
  const node = els.trackTemplate.content.firstElementChild.cloneNode(true);
  const query = selector => node.querySelector(selector);

  const lane = query('.track-lane');
  const canvas = query('.waveform');
  const track = {
    id: nextTrackId++,
    name,
    buffer: null,
    fileName: '',
    muted: false,
    solo: false,
    volume: 1,
    pan: 0,
    offset: 0,
    source: null,
    gainNode: null,
    panNode: null,
    element: node,
    lane,
    canvas,
    clipLabel: query('.clip-label'),
    playheadEl: query('.playhead-local'),
    fileData: null
  };

  const nameInput = query('.track-name');
  const muteBtn = query('.mute-btn');
  const soloBtn = query('.solo-btn');
  const volumeSlider = query('.volume-slider');
  const panSlider = query('.pan-slider');
  const offsetInput = query('.offset-input');
  const removeBtn = query('.remove-track');

  nameInput.value = name;
  nameInput.addEventListener('input', () => {
    track.name = nameInput.value || `Piste ${track.id}`;
  });

  muteBtn.addEventListener('click', () => {
    track.muted = !track.muted;
    muteBtn.classList.toggle('active', track.muted);
    updateActiveTrackGains();
  });

  soloBtn.addEventListener('click', () => {
    track.solo = !track.solo;
    soloBtn.classList.toggle('active', track.solo);
    updateActiveTrackGains();
  });

  volumeSlider.addEventListener('input', () => {
    track.volume = Number(volumeSlider.value);
    updateActiveTrackGains();
  });

  panSlider.addEventListener('input', () => {
    track.pan = Number(panSlider.value);
    track.panNode?.pan.setValueAtTime(track.pan, ensureAudio().currentTime);
  });

  offsetInput.addEventListener('change', () => {
    track.offset = Math.max(0, Number(offsetInput.value) || 0);
    offsetInput.value = track.offset.toFixed(1);
    if (playing) restartPlaybackAt(getCurrentPlayhead());
    updateProjectInfo();
  });

  removeBtn.addEventListener('click', () => {
    stopTrackSource(track);
    tracks = tracks.filter(t => t !== track);
    node.remove();
    updateProjectInfo();
    setStatus(`Piste ${track.name} supprimée`);
  });

  lane.addEventListener('click', event => {
    const rect = lane.getBoundingClientRect();
    const laneX = event.clientX - rect.left + lane.scrollLeft;
    seekTo(laneX / getPixelsPerSecond());
  });

  tracks.push(track);
  els.tracksContainer.appendChild(node);
  updateProjectInfo();
  return track;
}

function getAudibleFactor(track) {
  const anySolo = tracks.some(t => t.solo);
  if (track.muted) return 0;
  if (anySolo && !track.solo) return 0;
  return track.volume;
}

function updateActiveTrackGains() {
  if (!audioCtx) return;
  for (const track of tracks) {
    track.gainNode?.gain.setTargetAtTime(getAudibleFactor(track), audioCtx.currentTime, 0.01);
    track.panNode?.pan.setTargetAtTime(track.pan, audioCtx.currentTime, 0.01);
  }
}

async function loadFiles(fileList) {
  const files = [...fileList];
  if (!files.length) return;
  ensureAudio();
  setStatus(`Import de ${files.length} fichier(s)...`);

  for (const file of files) {
    try {
      const arrayBuffer = await file.arrayBuffer();
      const decoded = await audioCtx.decodeAudioData(arrayBuffer.slice(0));
      const track = createTrack(file.name.replace(/\.[^.]+$/, ''));
      track.buffer = decoded;
      track.fileName = file.name;
      track.fileData = arrayBuffer;
      track.clipLabel.textContent = `${file.name} · ${formatTime(decoded.duration, false)}`;
      renderWaveform(track);
    } catch (err) {
      console.error(err);
      setStatus(`Impossible de décoder ${file.name}`);
    }
  }
  updateProjectInfo();
  setStatus(`${files.length} fichier(s) importé(s)`);
}

function renderAllWaveforms() {
  for (const track of tracks) renderWaveform(track);
}

function renderWaveform(track) {
  const { canvas } = track;
  if (!canvas) return;

  const pps = getPixelsPerSecond();
  const width = Math.max(720, Math.ceil(state.projectDuration * pps));
  track.lane.style.width = `${width}px`;

  const dpr = window.devicePixelRatio || 1;
  const cssHeight = 118;
  canvas.style.width = `${width}px`;
  canvas.style.height = `${cssHeight}px`;
  canvas.width = Math.floor(width * dpr);
  canvas.height = Math.floor(cssHeight * dpr);

  const ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);
  ctx.clearRect(0, 0, width, cssHeight);

  // Grid
  ctx.strokeStyle = 'rgba(255,255,255,0.04)';
  ctx.lineWidth = 1;
  for (let s = 0; s <= state.projectDuration; s += 5) {
    const x = s * pps + 0.5;
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, cssHeight);
    ctx.stroke();
  }

  if (!track.buffer) {
    ctx.fillStyle = '#5f6b7b';
    ctx.font = '12px Segoe UI, Arial';
    ctx.fillText('Piste vide', 12, 65);
    return;
  }

  const xStart = track.offset * pps;
  const clipWidth = track.buffer.duration * pps;
  ctx.fillStyle = 'rgba(43, 112, 151, 0.24)';
  ctx.fillRect(xStart, 6, clipWidth, cssHeight - 12);
  ctx.strokeStyle = 'rgba(84, 184, 255, 0.55)';
  ctx.strokeRect(xStart + 0.5, 6.5, Math.max(1, clipWidth - 1), cssHeight - 13);

  const data = track.buffer.getChannelData(0);
  const sampleCount = Math.max(1, Math.floor(clipWidth));
  const samplesPerPixel = Math.max(1, Math.floor(data.length / sampleCount));
  const centerY = cssHeight / 2;
  const amp = (cssHeight - 24) / 2;

  ctx.strokeStyle = '#7fd0ff';
  ctx.lineWidth = 1;
  ctx.beginPath();
  for (let px = 0; px < sampleCount; px++) {
    const start = px * samplesPerPixel;
    const end = Math.min(data.length, start + samplesPerPixel);
    let min = 1;
    let max = -1;
    for (let i = start; i < end; i++) {
      const v = data[i];
      if (v < min) min = v;
      if (v > max) max = v;
    }
    const x = xStart + px;
    ctx.moveTo(x, centerY + min * amp);
    ctx.lineTo(x, centerY + max * amp);
  }
  ctx.stroke();

  track.clipLabel.style.left = `${xStart + 7}px`;
}

function stopTrackSource(track) {
  if (track.source) {
    track.source.onended = null;
    try { track.source.stop(); } catch { /* already stopped */ }
    try { track.source.disconnect(); } catch { /* not connected */ }
  }
  track.gainNode?.disconnect();
  track.panNode?.disconnect();
  track.source = null;
  track.gainNode = null;
  track.panNode = null;
}

function stopAllSources() {
  for (const track of tracks) stopTrackSource(track);
}

function scheduleTrack(track, projectOffset) {
  if (!track.buffer) return;
  const ctx = ensureAudio();
  const clipStart = track.offset;
  const clipEnd = track.offset + track.buffer.duration;
  if (projectOffset >= clipEnd) return;

  const source = ctx.createBufferSource();
  source.buffer = track.buffer;
  const gain = ctx.createGain();
  const pan = ctx.createStereoPanner();
  gain.gain.value = getAudibleFactor(track);
  pan.pan.value = track.pan;

  source.connect(gain);
  gain.connect(pan);
  pan.connect(masterGain);

  const when = projectOffset < clipStart ? ctx.currentTime + (clipStart - projectOffset) : ctx.currentTime;
  const sourceOffset = Math.max(0, projectOffset - clipStart);
  const duration = Math.max(0, track.buffer.duration - sourceOffset);
  if (duration > 0) source.start(when, sourceOffset, duration);

  track.source = source;
  track.gainNode = gain;
  track.panNode = pan;
}

function startPlayback(offset = pausedAt) {
  const ctx = ensureAudio();
  if (playing) return;
  if (tracks.every(t => !t.buffer)) {
    setStatus('Importe au moins un fichier audio avant la lecture');
    return;
  }

  offset = Math.max(0, Math.min(offset, state.projectDuration));
  stopAllSources();
  for (const track of tracks) scheduleTrack(track, offset);

  startedAt = ctx.currentTime - offset;
  pausedAt = offset;
  playing = true;
  els.playBtn.classList.add('active');
  setStatus('Lecture');
  startMetronomeScheduler();
  animationLoop();
}

function pausePlayback() {
  if (!playing) return;
  pausedAt = getCurrentPlayhead();
  state.lastKnownPlayhead = pausedAt;
  stopAllSources();
  playing = false;
  els.playBtn.classList.remove('active');
  stopMetronomeScheduler();
  cancelAnimationFrame(rafId);
  updatePlayhead(pausedAt);
  setStatus('Pause');
}

function stopPlayback(reset = true) {
  stopAllSources();
  playing = false;
  els.playBtn.classList.remove('active');
  stopMetronomeScheduler();
  cancelAnimationFrame(rafId);
  if (reset) pausedAt = 0;
  state.lastKnownPlayhead = pausedAt;
  updatePlayhead(pausedAt);
  setStatus('Stop');
}

function restartPlaybackAt(sec) {
  const wasPlaying = playing;
  stopAllSources();
  playing = false;
  stopMetronomeScheduler();
  pausedAt = sec;
  state.lastKnownPlayhead = sec;
  if (wasPlaying) startPlayback(sec);
  else updatePlayhead(sec);
}

function seekTo(sec) {
  sec = Math.max(0, Math.min(sec, state.projectDuration));
  restartPlaybackAt(sec);
  setStatus(`Curseur ${formatTime(sec)}`);
}

function getCurrentPlayhead() {
  if (!playing || !audioCtx) return pausedAt;
  return Math.max(0, audioCtx.currentTime - startedAt);
}

function updatePlayhead(sec) {
  state.lastKnownPlayhead = sec;
  els.timeDisplay.textContent = formatTime(sec);
  const x = sec * getPixelsPerSecond();
  for (const track of tracks) {
    track.playheadEl.style.left = `${x}px`;
  }
}

function animationLoop() {
  if (!playing) return;
  const sec = getCurrentPlayhead();
  if (sec >= state.projectDuration) {
    if (loopEnabled) {
      restartPlaybackAt(0);
      return;
    }
    stopPlayback(true);
    return;
  }
  updatePlayhead(sec);
  rafId = requestAnimationFrame(animationLoop);
}

function toggleLoop() {
  loopEnabled = !loopEnabled;
  els.loopBtn.classList.toggle('active', loopEnabled);
  setStatus(loopEnabled ? 'Boucle activée' : 'Boucle désactivée');
}

function toggleMetronome() {
  metronomeEnabled = !metronomeEnabled;
  els.metronomeBtn.classList.toggle('active', metronomeEnabled);
  if (playing) {
    stopMetronomeScheduler();
    startMetronomeScheduler();
  }
  setStatus(metronomeEnabled ? 'Métronome activé' : 'Métronome désactivé');
}

function startMetronomeScheduler() {
  stopMetronomeScheduler();
  if (!metronomeEnabled || !playing) return;
  const ctx = ensureAudio();
  const bpm = Math.max(30, Math.min(300, Number(els.bpmInput.value) || 120));
  const beatLen = 60 / bpm;
  const projectPos = getCurrentPlayhead();
  const beatIndex = Math.ceil(projectPos / beatLen);
  metronomeNextBeat = startedAt + beatIndex * beatLen;

  metronomeTimer = setInterval(() => {
    const lookAhead = 0.12;
    while (metronomeNextBeat < ctx.currentTime + lookAhead) {
      scheduleClick(metronomeNextBeat, beatIndex % 4 === 0);
      metronomeNextBeat += beatLen;
    }
  }, 25);
}

function stopMetronomeScheduler() {
  if (metronomeTimer) clearInterval(metronomeTimer);
  metronomeTimer = null;
}

function scheduleClick(when, accent) {
  const ctx = ensureAudio();
  const osc = ctx.createOscillator();
  const gain = ctx.createGain();
  osc.frequency.value = accent ? 1300 : 900;
  gain.gain.setValueAtTime(0.0001, when);
  gain.gain.exponentialRampToValueAtTime(0.17, when + 0.002);
  gain.gain.exponentialRampToValueAtTime(0.0001, when + 0.045);
  osc.connect(gain);
  gain.connect(masterGain);
  osc.start(when);
  osc.stop(when + 0.05);
}

async function exportWav() {
  const activeTracks = tracks.filter(t => t.buffer);
  if (!activeTracks.length) {
    setStatus('Aucune piste audio à exporter');
    return;
  }

  try {
    setStatus('Rendu du mix en cours...');
    const sampleRate = 44100;
    const duration = getProjectDuration();
    const offline = new OfflineAudioContext(2, Math.ceil(duration * sampleRate), sampleRate);
    const outGain = offline.createGain();
    outGain.gain.value = Number(els.masterVolume.value);
    outGain.connect(offline.destination);
    const anySolo = tracks.some(t => t.solo);

    for (const track of activeTracks) {
      const source = offline.createBufferSource();
      source.buffer = track.buffer;
      const gain = offline.createGain();
      const pan = offline.createStereoPanner();
      gain.gain.value = track.muted || (anySolo && !track.solo) ? 0 : track.volume;
      pan.pan.value = track.pan;
      source.connect(gain);
      gain.connect(pan);
      pan.connect(outGain);
      source.start(track.offset);
    }

    const rendered = await offline.startRendering();
    const wavBlob = audioBufferToWavBlob(rendered);
    const url = URL.createObjectURL(wavBlob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'SonoForge_Mix.wav';
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
    setStatus('Mix WAV exporté');
  } catch (err) {
    console.error(err);
    setStatus('Erreur pendant l’export WAV');
  }
}

function audioBufferToWavBlob(buffer) {
  const numChannels = Math.min(2, buffer.numberOfChannels || 1);
  const { sampleRate } = buffer;
  const format = 1;
  const bitDepth = 16;
  const bytesPerSample = bitDepth / 8;
  const blockAlign = numChannels * bytesPerSample;
  const dataLength = buffer.length * blockAlign;
  const ab = new ArrayBuffer(44 + dataLength);
  const view = new DataView(ab);
  let offset = 0;

  const writeString = s => { for (const ch of s) view.setUint8(offset++, ch.charCodeAt(0)); };
  const write16 = v => { view.setUint16(offset, v, true); offset += 2; };
  const write32 = v => { view.setUint32(offset, v, true); offset += 4; };

  writeString('RIFF'); write32(36 + dataLength); writeString('WAVE');
  writeString('fmt '); write32(16); write16(format); write16(numChannels);
  write32(sampleRate); write32(sampleRate * blockAlign); write16(blockAlign); write16(bitDepth);
  writeString('data'); write32(dataLength);

  const ch0 = buffer.getChannelData(0);
  const ch1 = numChannels > 1 ? buffer.getChannelData(1) : ch0;
  for (let i = 0; i < buffer.length; i++) {
    for (let ch = 0; ch < numChannels; ch++) {
      const sample = Math.max(-1, Math.min(1, ch === 0 ? ch0[i] : ch1[i]));
      view.setInt16(offset, sample < 0 ? sample * 0x8000 : sample * 0x7fff, true);
      offset += 2;
    }
  }
  return new Blob([view], { type: 'audio/wav' });
}

els.fileInput.addEventListener('change', async () => {
  await loadFiles(els.fileInput.files);
  els.fileInput.value = '';
});

els.addEmptyTrackBtn.addEventListener('click', () => {
  createTrack();
  setStatus('Piste vide ajoutée');
});

els.playBtn.addEventListener('click', () => startPlayback(pausedAt));
els.pauseBtn.addEventListener('click', pausePlayback);
els.stopBtn.addEventListener('click', () => stopPlayback(true));
els.rewindBtn.addEventListener('click', () => seekTo(0));
els.loopBtn.addEventListener('click', toggleLoop);
els.metronomeBtn.addEventListener('click', toggleMetronome);
els.exportBtn.addEventListener('click', exportWav);

els.masterVolume.addEventListener('input', () => {
  masterGain?.gain.setTargetAtTime(Number(els.masterVolume.value), audioCtx?.currentTime ?? 0, 0.01);
});

els.bpmInput.addEventListener('change', () => {
  els.bpmInput.value = String(Math.max(30, Math.min(300, Number(els.bpmInput.value) || 120)));
  if (playing && metronomeEnabled) startMetronomeScheduler();
});

els.zoomRange.addEventListener('input', () => {
  // Slider élevé = zoom plus fort = moins de secondes pour 100 px.
  const v = Number(els.zoomRange.value);
  secondsPer100px = Math.max(2, Math.min(14, 14 - ((v - 60) / 200) * 12));
  updateProjectInfo();
});

els.ruler.addEventListener('click', event => {
  const rect = els.ruler.getBoundingClientRect();
  const x = event.clientX - rect.left + els.ruler.parentElement.scrollLeft;
  seekTo(x / getPixelsPerSecond());
});

const DRAG_ACTIVE_BY_EVENT = {
  dragenter: true,
  dragover: true,
  dragleave: false,
  drop: false
};

for (const [eventName, isActive] of Object.entries(DRAG_ACTIVE_BY_EVENT)) {
  els.dropZone.addEventListener(eventName, event => {
    event.preventDefault();
    els.dropZone.classList.toggle('drag-over', isActive);
  });
}

els.dropZone.addEventListener('drop', async event => {
  const audioFiles = [...event.dataTransfer.files]
    .filter(f => f.type.startsWith('audio/') || /\.(wav|mp3|m4a|ogg|flac)$/i.test(f.name));
  await loadFiles(audioFiles);
});

window.addEventListener('resize', () => {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(renderAllWaveforms, 150);
});

// Initialisation visuelle
createTrack('Audio 1');
createTrack('Audio 2');
renderRuler();
updatePlayhead(0);
setStatus('Prêt - importe un fichier audio');
