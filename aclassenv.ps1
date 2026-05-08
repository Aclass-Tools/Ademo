param(
    [Parameter(Position = 0)]
    [string]$Command,

    [Parameter(Position = 1)]
    [string]$Mode
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $ScriptDir "scripts/cmake-tools.ps1")

if ($PSBoundParameters.ContainsKey("Command")) {
    Acli -Command $Command -Mode $Mode
}
