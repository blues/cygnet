set -x

jq -n --slurpfile input input.json '{
  "imsiList": [ $input[] | {
    "deviceIdentifier": .device,
    "iccid":"",
    "imei":"",
    "imsi": .device | sub("skylo:"; ""),
    "niddApnList": ["blues.prod", "blues.staging", "blues.dev"]
    }],
    "regionCodes": ["GLOBAL"],
    "serviceType": "IOT"
}' > output.json
