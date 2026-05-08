# Scripts

This directory stores helper scripts for both Bash and PowerShell.

## Files

- `cmake-tools.sh`: Bash build helpers
- `cmake-tools.ps1`: PowerShell build helpers

## PowerShell usage (defaults shown)

```powershell
. .\aclassenv.ps1
Abuild
Abuild Release
Ainstall
Adist
Apack
Aclean
```

Direct PowerShell CLI mode:

```powershell
.\aclassenv.ps1 build
.\aclassenv.ps1 build release
.\aclassenv.ps1 install
.\aclassenv.ps1 dist
.\aclassenv.ps1 pack
.\aclassenv.ps1 clean
```

## Bash usage (defaults shown)

```bash
source ./aclassenv.sh
Abuild
Abuild Release
Ainstall
Adist
Apack
Aclean
```
