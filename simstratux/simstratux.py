from flask import Flask, jsonify
import threading
import time
from pynput import keyboard
import math

app = Flask(__name__)

# Conversion of DMS (Degrees, Minutes, Seconds) to Decimal Degrees
def dms_to_dd(degrees, minutes, seconds, direction):
    dd = float(degrees) + float(minutes)/60 + float(seconds)/(60*60)
    if direction == 'S' or direction == 'W':
        dd *= -1
    return dd

# Example conversion for Góra Ślęża coordinates
latitude = dms_to_dd(50, 51, 55, 'N')
longitude = dms_to_dd(16, 42, 41, 'E')


# Updated initial data with EPWS (Wrocław-Szymanów Airport) coordinates in Decimal Degrees
data = {
    "GPSLatitude": latitude,  # Updated to EPWS latitude
    "GPSLongitude": longitude,  # Updated to EPWS longitude
    "GPSHeightAboveEllipsoid": 500,
    "AHRSRoll": 0,
    "AHRSPitch": 0,
    "AHRSGyroHeading": 0,
    "AHRSMagHeading": 0,
    "GPSVerticalSpeed": -0.6135171,
    "BaroVerticalSpeed": 1.3123479
}

def simulate_aircraft_movement():
    while True:
        # These might now be updated directly by keyboard listeners
        time.sleep(1)  # Just to slow down the loop for this example

def on_press(key):
    try:
        heading_rad = math.radians(data["AHRSGyroHeading"])
        # Movement step for latitude and longitude
        movement_step = 0.001
        # Calculate the change based on the heading
        delta_lat = movement_step * math.cos(heading_rad)
        delta_lon = movement_step * math.sin(heading_rad)

        if key.char == 'w':
            data["GPSLatitude"] += delta_lat
            data["GPSLongitude"] += delta_lon
        elif key.char == 's':
            data["GPSLatitude"] -= delta_lat
            data["GPSLongitude"] -= delta_lon
        elif key.char == 'a':
            # For strafing, rotate the heading by 90 degrees (left)
            delta_lat_strafe = movement_step * math.cos(heading_rad - math.pi / 2)
            delta_lon_strafe = movement_step * math.sin(heading_rad - math.pi / 2)
            data["GPSLatitude"] += delta_lat_strafe
            data["GPSLongitude"] += delta_lon_strafe
        elif key.char == 'd':
            # For strafing, rotate the heading by 90 degrees (right)
            delta_lat_strafe = movement_step * math.cos(heading_rad + math.pi / 2)
            delta_lon_strafe = movement_step * math.sin(heading_rad + math.pi / 2)
            data["GPSLatitude"] += delta_lat_strafe
            data["GPSLongitude"] += delta_lon_strafe

        # Adjust AHRSRoll and AHRSPitch with 'J', 'L', 'I', 'K'
        elif key.char == 'j':
            data["AHRSRoll"] -= 2
        elif key.char == 'l':
            data["AHRSRoll"] += 2
        elif key.char == 'i':
            data["AHRSPitch"] += 2
        elif key.char == 'k':
            data["AHRSPitch"] -= 2

        # Adjust AHRSGyroHeading with 'Q' and 'E'
        elif key.char == 'q':
            data["AHRSGyroHeading"] = (data["AHRSGyroHeading"] - 5) % 360
        elif key.char == 'e':
            data["AHRSGyroHeading"] = (data["AHRSGyroHeading"] + 5) % 360

        # Adjust GPSHeightAboveEllipsoid (altitude) with 'U' and 'O'
        elif key.char == 'u':
            data["GPSHeightAboveEllipsoid"] += 10
        elif key.char == 'o':
            data["GPSHeightAboveEllipsoid"] -= 10
    except AttributeError:
        pass


@app.route('/getSituation', methods=['GET'])
def get_situation():
    return jsonify(data)

if __name__ == '__main__':
    # Start the simulation in a background thread
    threading.Thread(target=simulate_aircraft_movement, daemon=True).start()

    # Set up and start keyboard listener in a non-blocking manner
    keyboard_listener = keyboard.Listener(on_press=on_press)
    keyboard_listener.start()

    # Start Flask app
    # Note: Use threaded=True to allow Flask to handle requests in parallel
    # with the background simulation and input listeners.
    app.run(debug=True, threaded=True)
