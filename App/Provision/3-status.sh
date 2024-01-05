set -x

curl -si -X GET --location 'https://api-sim.ntnyolks.space/sim-api/sim-profile/activation/view-result?request_number='$1 \
--header 'X-ENTITY-ID: '$SKYLO_ENTITY_ID \
--header 'X-APIKEY: '$SKYLO_API_KEY
