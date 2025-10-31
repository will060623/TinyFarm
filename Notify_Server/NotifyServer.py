from flask import Flask, request, jsonify, make_response
from time import time, sleep
import serial
import threading
import json

PORT = '/dev/ttyACM0'        #check your USB Port name
BAUD = 115200

arduino = serial.Serial(PORT, BAUD, timeout=0.1, write_timeout=0.5)

app = Flask(__name__)

def send_to_arduino(line: str):
    try:
        arduino.write(line.encode('utf-8'))
    except serial.SerialTimeoutException:
        print("[SER] write timeout — dropped:", line.strip())
    except serial.SerialException as e:
        print("[SER] serial error:", e)

@app.route('/notify', methods=['POST'])
def notify():
    try:
        data = request.get_json(silent=True) 
        if not data:
            print("Request body is empty.")
            resp = make_response(jsonify({"error": "Empty request body"}), 400)
            resp.headers["X-M2M-RSC"] = "4000"  
            return resp
        else:
            print("Notify Message Arrived!")

            req_id = request.headers.get("X-M2M-RI", "")

            m2m_sgn = data.get("m2m:sgn")
            if not m2m_sgn:
                print("Missing 'm2m:sgn' key.")
                resp = make_response(jsonify({"error": "Invalid JSON structure"}), 400)
                resp.headers["X-M2M-RSC"] = "4000"
                if req_id: resp.headers["X-M2M-RI"] = req_id
                return resp


            if m2m_sgn.get("vrq") is True:
                print("Verification request received (vrq=true).")
                resp = make_response("", 200)
                resp.headers["X-M2M-RSC"] = "2000"
                if req_id: resp.headers["X-M2M-RI"] = req_id
                return resp

            sur = m2m_sgn.get("sur")
            nev = m2m_sgn.get("nev")
            if not nev:
                print("Missing 'nev' key.")
                resp = make_response(jsonify({"error": "Invalid JSON structure"}), 400)
                resp.headers["X-M2M-RSC"] = "4000"
                if req_id: resp.headers["X-M2M-RI"] = req_id
                return resp

            rep_val = nev.get("rep")
            if not rep_val:
                print("Missing 'rep' key.")
                resp = make_response(jsonify({"error": "Invalid JSON structure"}), 400)
                resp.headers["X-M2M-RSC"] = "4000"
                if req_id: resp.headers["X-M2M-RI"] = req_id
                return resp

            if isinstance(rep_val, str):
                rep = json.loads(rep_val)
            elif isinstance(rep_val, dict):
                rep = rep_val
            else:
                print("Invalid 'rep' type.")
                resp = make_response(jsonify({"error": "Invalid JSON structure"}), 400)
                resp.headers["X-M2M-RSC"] = "4000"
                if req_id: resp.headers["X-M2M-RI"] = req_id
                return resp

            m2m_cin = rep.get("m2m:cin")
            if not m2m_cin:
                print("Missing 'm2m:cin' key.")
                resp = make_response(jsonify({"error": "Invalid JSON structure"}), 400)
                resp.headers["X-M2M-RSC"] = "4000"
                if req_id: resp.headers["X-M2M-RI"] = req_id
                return resp

            con = m2m_cin.get("con")
            if con:
                last_seg = ""
                if isinstance(sur, str) and sur:
                    last_seg = sur.rsplit('/', 1)[-1]

                if sur == "TinyIoT/TinyFarm/Actuators/LED/LEDsub":
                    print(f"{sur} -> {last_seg} : {con}")
                    signal = f"{last_seg}:{con}\n"
                    threading.Thread(target=send_to_arduino, args=(signal,), daemon=True).start()

                    resp = make_response(jsonify({"message": "Motor activation initiated"}), 200)
                    resp.headers["X-M2M-RSC"] = "2000"
                    if req_id: resp.headers["X-M2M-RI"] = req_id
                    return resp

                elif sur == "TinyIoT/TinyFarm/Actuators/Door/Doorsub":
                    print(f"{sur} -> {last_seg} : {con}")
                    signal = f"{last_seg}:{con}\n"
                    threading.Thread(target=send_to_arduino, args=(signal,), daemon=True).start()

                    resp = make_response(jsonify({"message": "Motor activation initiated"}), 200)
                    resp.headers["X-M2M-RSC"] = "2000"
                    if req_id: resp.headers["X-M2M-RI"] = req_id
                    return resp

                elif sur == "TinyIoT/TinyFarm/Actuators/Fan/Fansub":
                    print(f"{sur} -> {last_seg} : {con}")
                    signal = f"{last_seg}:{con}\n"
                    threading.Thread(target=send_to_arduino, args=(signal,), daemon=True).start()

                    resp = make_response(jsonify({"message": "Motor activation initiated"}), 200)
                    resp.headers["X-M2M-RSC"] = "2000"
                    if req_id: resp.headers["X-M2M-RI"] = req_id
                    return resp

                elif sur == "TinyIoT/TinyFarm/Actuators/Water/Watersub":
                    print(f"{sur} -> {last_seg} : {con}")
                    signal = f"{last_seg}:{con}\n"
                    threading.Thread(target=send_to_arduino, args=(signal,), daemon=True).start()

                    resp = make_response(jsonify({"message": "Motor activation initiated"}), 200)
                    resp.headers["X-M2M-RSC"] = "2000"
                    if req_id: resp.headers["X-M2M-RI"] = req_id
                    return resp

            resp = make_response(jsonify({"message": "ignored or missing con"}), 200)
            resp.headers["X-M2M-RSC"] = "2000"
            if req_id: resp.headers["X-M2M-RI"] = req_id
            return resp

    except json.JSONDecodeError as e:
        print(f"JSON decoding error: {e}")
        resp = make_response(jsonify({"error": "Invalid JSON"}), 400)
        resp.headers["X-M2M-RSC"] = "4000"
        return resp

    except Exception as e:
        print(f"An error occurred: {e}")
        resp = make_response(jsonify({"error": "Internal server error"}), 500)
        return resp

if __name__ == '__main__':                           #Notify Server
    app.run(host='0.0.0.0', port=8866, threaded=True)

