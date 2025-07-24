import xml.etree.ElementTree as ET
import re


def get_path_from_svg(filename: str) -> list[str] | None:
    """
    Get data for all paths present in svg file.

    :param filename: path to svg file

    :return ret_list: return a list of path data
    """
    tree = ET.parse(filename)

    root = tree.getroot()

    ns = {"svg" : "http://www.w3.org/2000/svg"}

    ret = root.findall(".//svg:path", ns)

    if ret is None:
        print("Failed to find any paths")
        return None
    
    ret_list = []

    for path in ret:
        d_attr = path.attrib["d"]
        ret_list.append(d_attr)    

    return ret_list



def get_pts_from_svg(filename: str) -> list[list]:
    """
    Get points from paths in svg file.

    :param filename: path to svg file.

    :return ret_list: list of path data as [[[x0,y0], [x1,y1]], [...], ...]. If
        no paths are available it returns an empty list.
    """
    path_list = get_path_from_svg(filename)

    if path_list is None:
        print(f"No paths were found in {filename}")
        return []
    
    def next_float():
        nonlocal idx
        val = float(tokens[idx])
        idx += 1
        return val
    
    ret_vals = []

    for path in path_list:

        # https://www.rexegg.com/regex-quickstart.php
        tokens = re.findall(r'[a-zA-Z]|-?\d+(?:\.\d+)?', path)
        idx = 0
        positions = []
        cur_pos = [0, 0]
        cmd = None

        while idx < len(tokens):
            val = tokens[idx]

            if re.match(r"[a-zA-Z]", val):
                cmd = val
                idx += 1

            match cmd:
                case "m":
                    cur_pos[0] += next_float()
                    cur_pos[1] += next_float()
                    # the following tokens are treated as relative lineto values
                    cmd = "l"
                    positions.append(cur_pos.copy())
                case "M":
                    cur_pos[0] = next_float()
                    cur_pos[1] = next_float()
                    # the following tokens are treated as absolute lineto values
                    cmd = "L"
                    positions.append(cur_pos.copy())
                case "l":
                    cur_pos[0] += next_float()
                    cur_pos[1] += next_float()
                    positions.append(cur_pos.copy())
                case "L":
                    cur_pos[0] = next_float()
                    cur_pos[1] = next_float()
                    positions.append(cur_pos.copy())
                case "v":
                    cur_pos[1] += next_float()
                    positions.append(cur_pos.copy())
                case "V":
                    cur_pos[1] = next_float()
                    positions.append(cur_pos.copy())
                case "h":
                    cur_pos[0] += next_float()
                    positions.append(cur_pos.copy())
                case "H":
                    cur_pos[0] = next_float()
                    positions.append(cur_pos.copy())
                case "z":
                    cur_pos = positions[0]
                    positions.append(cur_pos.copy())
                case "Z":
                    cur_pos = positions[0]
                    positions.append(cur_pos.copy())
                case _:
                    print(f"Received unsupported command {cmd}")
                    break
    
        if len(positions) != 0:
            ret_vals.append(positions)

    
    return ret_vals