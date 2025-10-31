import os
import sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from fastapi import FastAPI, Request, Form
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles

from constants import EngineSubmission, ButtonPress
from utils import load_json
import stlib as st
import numpy as np


id_map = load_json()
app = FastAPI()

worker = st.Worker(COM="COM9")
worker.start_worker()
worker.start()
worker.home()

# Serve static files (HTML, CSS, JS, and images)
app.mount("/static", StaticFiles(directory="static"), name="static")


@app.get("/", response_class=HTMLResponse)
async def index():
    with open("static/index.html") as f:
        return HTMLResponse(f.read())


@app.post("/submit")
async def submit(data: EngineSubmission):
    
    match data.engine:
        case "PathMaker":
            print(f"Got pathmaker: rot->{data.rotate}° n->{data.rotations}")
            name = id_map[data.item_id]
            fname = f"static/images/{name}/source.svg"
            pts = st.get_pts_from_svg(fname)
            pts = np.array(pts)
            pm = st.PathMaker(pts, eps=1, rot_angle=data.rotate, 
                         num_iterations=data.rotations)
            worker.add_PathMaker(pm)

        case "SpiralAboutCenter":
            print(f"Got spiral: n->{data.rotations} r0->{data.r0} r1->{data.r1}")
            pm = st.SpiralAboutCenter(r0 = data.r0, r1 = data.r1, 
                                      num_revolutions = data.rotations)
            worker.add_PathMaker(pm)

        case _:
            print(f"Received unexpected engine {data.engine}")


@app.post("/button")
async def button_press(data: ButtonPress):
    match data.task:
        case "home":
            # print("home")
            worker.home()
        case "start":
            # print("start")
            worker.start()
        case "stop":
            # print("stop")
            worker.stop()
        case "clear":
            # print("clear")
            worker.stop(clear=True)
        case _:
            print(f"Received unexpected {data.task}")