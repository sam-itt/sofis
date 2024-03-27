from flask import Flask, jsonify, render_template_string
import gpxpy
import gpxpy.gpx
import threading

app = Flask(__name__)

# Function to parse a GPX file and extract data for a specific point
def parse_gpx_point(file_path, point_index):
    with open(file_path, 'r') as gpx_file:
        gpx = gpxpy.parse(gpx_file)

    # Accessing tracks, segments, and points safely
    if gpx.tracks and gpx.tracks[0].segments and len(gpx.tracks[0].segments[0].points) > point_index:
        point = gpx.tracks[0].segments[0].points[point_index]
        data = {
            "GPSLatitude": point.latitude,
            "GPSLongitude": point.longitude,
            "GPSHeightAboveEllipsoid": point.elevation,
            # Placeholder for fields not available in GPX
            "AHRSRoll": None,
            "AHRSPitch": None,
            "AHRSGyroHeading": None,
            "AHRSMagHeading": None,
            "GPSVerticalSpeed": None,
            "BaroVerticalSpeed": None
        }
    else:
        data = {}
    return data

# Keeping track of the current point index
current_point_index = -1  # Start before the first point

@app.route('/getSituation', methods=['GET'])
def get_situation():
    global current_point_index
    # Update the path to your GPX file
    file_path = 'epws.gpx'
    current_point_index += 1  # Move to the next point
    data = parse_gpx_point(file_path, current_point_index)
    if not data:  # If there are no more points, start over
        current_point_index = 0
        data = parse_gpx_point(file_path, current_point_index)
    return jsonify(data)

if __name__ == '__main__':
    app.run(debug=True)
