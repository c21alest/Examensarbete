import requests
import json
import time

url = "http://localhost:8080/api/clients/battery-sensor/3411/0/1?timeout=5&format=TLV"
#url = "http://localhost:8081/http://localhost:8080/api/clients/battery-sensor/3411/0/1?timeout=5&format=TLV" # With CORS Proxy

iterations = 20
stallFor = 1 # seconds
data = []

for i in range(iterations):
    start_time = time.time() # Start timer
    response = requests.get(url)
    end_time = time.time()
    fetch_time = (end_time - start_time) * 1000 # Calculate elapsed time in ms

    # Get value from response
    response_json = response.json()
    value = response_json['content']['value']

    # Add result to JSON
    iteration_data = {
        "time": fetch_time,
        "value": value
    }
    data.append(iteration_data)

    # Stall/Pause
    time.sleep(stallFor)

# Create JSON file with result
with open('performance_data.json', 'w') as outfile:
    json.dump(data, outfile, indent=4)

print("Data har sparats i performance_data.json")
