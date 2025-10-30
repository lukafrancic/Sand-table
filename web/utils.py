import os
import json


def load_json() -> dict:
    names = os.listdir("static/images")

    data = []
    id_map = {}

    for i, name in enumerate(names):
        id_map[i+1] = name
        data.append({"id": i+1, "name": name})

    with open("static/items.json", "w") as file:
        json.dump(data, file, indent=2)

    return id_map
