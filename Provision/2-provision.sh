set +x
curl -si --location 'https://api-sim.ntnyolks.space/sim-api/sim-profile/activation/bulk' \
--header 'X-ENTITY-ID: '$SKYLO_ENTITY_ID \
--header 'X-APIKEY: '$SKYLO_API_KEY \
--header 'Content-Type: application/json' \
--data-binary @output.json \
-o response.json

set -x
cat response.json

