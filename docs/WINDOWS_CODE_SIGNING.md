# Signature Windows de SonoForge

Le workflow .github/workflows/windows-release.yml compile SonoForge sous Windows x64 et signe SonoForge.exe si un certificat de signature de code est configure.

## Secrets GitHub requis

Ajouter dans les secrets Actions du depot :

- WINDOWS_CERTIFICATE_BASE64 : contenu du certificat PFX converti en Base64.
- WINDOWS_CERTIFICATE_PASSWORD : mot de passe du fichier PFX.

Ne jamais ajouter le fichier PFX ou son mot de passe au depot.

## Conversion locale du PFX en Base64

PowerShell :

~~~powershell
[Convert]::ToBase64String([IO.File]::ReadAllBytes("sonoforge-signing.pfx")) | Set-Content certificate-base64.txt
~~~

Copier le contenu de certificate-base64.txt dans WINDOWS_CERTIFICATE_BASE64.

## Fonctionnement

Le workflow peut etre lance manuellement depuis GitHub Actions et se lance aussi pour un tag v*.

Sans certificat configure, le build reste fonctionnel mais l'executable n'est pas signe.

Avec les deux secrets presents, le certificat est reconstruit uniquement sur le runner temporaire, SonoForge.exe est signe en SHA-256 avec horodatage RFC 3161, la signature Authenticode est verifiee, puis l'executable est publie comme artifact. Pour un tag de version, il est egalement joint a la GitHub Release.

## Exemple de release

~~~powershell
git tag v0.2.2
git push origin v0.2.2
~~~
