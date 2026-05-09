# =========================================================
# THERMAL AI CLASSIFICATION
# RASPBERRY PI 3 B+
# MLX90640 + TFLITE
# CLEAN UI VERSION
# =========================================================

import time
import cv2
import numpy as np

import board
import busio
import adafruit_mlx90640

from tflite_runtime.interpreter import Interpreter


# =========================================================
# LOAD MODEL
# =========================================================

MODEL_PATH = "MLX_compatible.tflite"

interpreter = Interpreter(
    model_path=MODEL_PATH
)

interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("===================================")
print("AI MODEL LOADED")
print("===================================")


# =========================================================
# CLASS LABELS
# =========================================================

class_names = [

    "berdiri",
    "duduk",
    "jatuh belakang",
    "jatuh depan"

]


# =========================================================
# INIT MLX90640
# =========================================================

i2c = busio.I2C(

    board.SCL,
    board.SDA,

    frequency=400000

)

mlx = adafruit_mlx90640.MLX90640(i2c)

mlx.refresh_rate = (

    adafruit_mlx90640.RefreshRate.REFRESH_4_HZ

)

frame = np.zeros((24 * 32,))

print("===================================")
print("MLX90640 READY")
print("===================================")


# =========================================================
# FPS TIMER
# =========================================================

prev_time = time.time()


# =========================================================
# MAIN LOOP
# =========================================================

while True:

    try:

        # =================================================
        # GET THERMAL FRAME
        # =================================================

        mlx.getFrame(frame)

        thermal_array = np.array(frame)

        thermal_array = thermal_array.reshape((24, 32))


        # =================================================
        # NORMALIZE
        # =================================================

        img = cv2.normalize(

            thermal_array,

            None,

            0,

            255,

            cv2.NORM_MINMAX

        )

        img = np.uint8(img)


        # =================================================
        # APPLY COLORMAP
        # =================================================

        thermal = cv2.applyColorMap(

            img,

            cv2.COLORMAP_INFERNO

        )


        # =================================================
        # AI INPUT
        # =================================================

        ai_input = cv2.resize(

            thermal,

            (64, 64)

        )


        # =================================================
        # NORMALIZE AI INPUT
        # =================================================

        ai_input = ai_input.astype(

            np.float32

        ) / 255.0


        # =================================================
        # INPUT SHAPE
        # =================================================

        ai_input = np.expand_dims(

            ai_input,

            axis=0

        )


        # =================================================
        # RUN INFERENCE
        # =================================================

        interpreter.set_tensor(

            input_details[0]['index'],

            ai_input

        )

        interpreter.invoke()


        # =================================================
        # GET PREDICTION
        # =================================================

        prediction = interpreter.get_tensor(

            output_details[0]['index']

        )[0]

        pred_index = np.argmax(prediction)

        confidence = prediction[pred_index]

        label = class_names[pred_index]


        # =================================================
        # RESIZE DISPLAY
        # =================================================

        thermal_big = cv2.resize(

            thermal,

            (640, 480),

            interpolation=cv2.INTER_NEAREST

        )


        # =================================================
        # FPS CALCULATION
        # =================================================

        current_time = time.time()

        fps = 1 / (current_time - prev_time)

        prev_time = current_time


        # =================================================
        # SMALL TOP CENTER TEXT
        # =================================================

        text = f"{label}  {confidence*100:.1f}%"

        font = cv2.FONT_HERSHEY_SIMPLEX

        font_scale = 0.45

        thickness = 1

        text_size = cv2.getTextSize(

            text,

            font,

            font_scale,

            thickness

        )[0]

        text_x = int((640 - text_size[0]) / 2)

        text_y = 20


        # =================================================
        # TEXT BACKGROUND
        # =================================================

        cv2.rectangle(

            thermal_big,

            (text_x - 8, text_y - 15),

            (text_x + text_size[0] + 8, text_y + 5),

            (0, 0, 0),

            -1

        )


        # =================================================
        # CLASSIFICATION TEXT
        # =================================================

        cv2.putText(

            thermal_big,

            text,

            (text_x, text_y),

            font,

            font_scale,

            (255, 255, 255),

            thickness,

            cv2.LINE_AA

        )


        # =================================================
        # FPS TEXT
        # =================================================

        cv2.putText(

            thermal_big,

            f"FPS {fps:.1f}",

            (10, 20),

            cv2.FONT_HERSHEY_SIMPLEX,

            0.35,

            (180, 180, 180),

            1,

            cv2.LINE_AA

        )


        # =================================================
        # SHOW WINDOW
        # =================================================

        cv2.imshow(

            "THERMAL AI",

            thermal_big

        )


        # =================================================
        # TERMINAL INFO
        # =================================================

        print(

            f"[INFO] "

            f"{label} | "

            f"{confidence:.2f} | "

            f"FPS {fps:.1f}"

        )


        # =================================================
        # EXIT
        # =================================================

        key = cv2.waitKey(1)

        if key == 27:
            break


    except Exception as e:

        print("MLX ERROR:", e)

        time.sleep(0.2)

        continue


# =========================================================
# CLEANUP
# =========================================================

cv2.destroyAllWindows()