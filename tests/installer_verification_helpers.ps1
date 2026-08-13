function Get-OptionalRegistryValue {
    param(
        [Parameter(Mandatory = $true)]
        [string]$LiteralPath,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    try {
        $value = Get-ItemPropertyValue -LiteralPath $LiteralPath -Name $Name -ErrorAction Stop
    } catch [System.Management.Automation.ItemNotFoundException] {
        return [pscustomobject]@{
            Exists = $false
            Value = $null
        }
    } catch [System.Management.Automation.PSArgumentException] {
        return [pscustomobject]@{
            Exists = $false
            Value = $null
        }
    }

    return [pscustomobject]@{
        Exists = $true
        Value = $value
    }
}
