# See: https://github.com/PowerShell/PowerShell/issues/2896
function Get-RedirectedUrl() {
  param(
    [Parameter(Mandatory = $true, Position = 0)]
    [uri] $url,
    [Parameter(Position = 1)]
    [Microsoft.PowerShell.Commands.WebRequestSession] $session = $null
  )

  $request_url = $url
  $retry = $false

  do {
    try {
      $response = Invoke-WebRequest -Method Head -WebSession $session -Uri $request_url

      if($response.BaseResponse.ResponseUri -ne $null)
      {
        # PowerShell 5
        $result = $response.BaseResponse.ResponseUri.AbsoluteUri
      } elseif ($response.BaseResponse.RequestMessage.RequestUri -ne $null) {
        # PowerShell Core
        $result = $response.BaseResponse.RequestMessage.RequestUri.AbsoluteUri
      }

      $retry = $false
    } catch {
      if(($_.Exception.GetType() -match "HttpResponseException") -and
        ($_.Exception -match "302"))
      {
        $request_url = $_.Exception.Response.Headers.Location.AbsoluteUri
        $retry = $true
      } else {
        throw $_
      }
    }  
  } while($retry)

  return $result
}

Function Req {
   Param(
       [Parameter(Mandatory=$True)]
       [hashtable]$Params,
       [int]$Retries = 1,
       [int]$SecondsDelay = 2
   )

   $method = $Params['Method']
   $url = $Params['Uri']

   $cmd = { Write-Verbose "$method $url..."; Invoke-WebRequest @Params }

   $retryCount = 0
   $completed = $false
   $response = $null

   while (-not $completed) {
       try {
           $response = Invoke-Command $cmd -ArgumentList $Params
           # if ($response.StatusCode -ne 200) {
           #     throw "Expecting reponse code 200, was: $($response.StatusCode)"
           # }
           $completed = $true
       } catch {
           Write-Verbose $_.Exception.GetType().FullName
           Write-Verbose $_.Exception.Message
           if ($retrycount -ge $Retries) {
               Write-Warning "Request to $url failed the maximum number of $retryCount times."
               throw
           } else {
               Write-Warning "Request to $url failed. Retrying in $SecondsDelay seconds."
               Start-Sleep $SecondsDelay
               $retrycount++
           }
       }
   }

   Write-Verbose "OK ($($response.StatusCode))"
   return $response
}
