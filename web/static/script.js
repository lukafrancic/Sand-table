let selectedId = null;
let selectedMeta = null;
let timeout;

async function loadItems() {
  const imageData = await fetch("/static/items.json");
  const items = await imageData.json();
  
  const container = document.getElementById("items");
  items.forEach((item) => {
    const div = document.createElement("div");
    div.className = "item";
    div.innerHTML = `
      <img src="/static/images/${item.name}/preview.png" alt="Item ${item.name}">
      <p>${item.name}</p>`;
    div.onclick = () => selectItem(item.id, item.name, div);
    container.appendChild(div);
  });

  const speedVal = document.querySelector("#speed-value");
  const speedInput = document.querySelector("#speed-slider");
  speedInput.addEventListener("input", (event) => {
    const val = event.target.value;
    speedVal.textContent = val;

    clearTimeout(timeout);
      timeout = setTimeout(() => {
        updateSpeed(val);
    }, 1000);
  });
}

async function selectItem(id, name, element) {
  selectedId = id;
  document.querySelectorAll(".item").forEach((el) => el.classList.remove("selected"));
  element.classList.add("selected");
  
  const file = await fetch(`/static/images/${name}/meta.json`);
  const meta = await file.json();

  selectedMeta = meta;
  showParameters(meta.parameters);
}

function showParameters(params) {
    console.log("Params", params);
    const container = document.getElementById("params");
    container.innerHTML = "";

    params.forEach(p=> {
        let html = "";

        html = `
          <label>${p.name}</label>
          <input type="number" id="${p.name}" value="">
        `;

        container.innerHTML += `<div class="param">${html}</div>`;
    });
}


async function postData(url, data) {
  try {
    const res = await fetch(url, {
      method: "POST",
      headers: {"Content-Type": "application/json"},
      body: JSON.stringify(data)
    });
  } catch (err) {
    alert("Error in POST ${url}:'", err);
  }
}


async function sendData() {
  if (!selectedId) {
    alert("Please select an item first!");
    return;
  }

  const payload = {
    item_id: selectedId,
    engine: selectedMeta.engine
  };

  selectedMeta.parameters.forEach(p=>{
    payload[p.name] = document.getElementById(p.name).value;
  });
  
  await postData("/submit", payload);
}


async function buttonPress(task) {
  const payload = {
    task: task  
  };

  await postData("/button", payload)
}


async function home() {
    await buttonPress("home");
}


async function start() {
    await buttonPress("start");
}


async function stop() {
    await buttonPress("stop");
}


async function clearQueue() {
    await buttonPress("clear");
}


async function shutdownDevice() {
  if (window.confirm("Do you really want to shutdown the device?")) {
    await buttonPress("shutdown");
  }
}


async function updateSpeed(speed) {
  const payload = {
    speed: speed  
  };
  await postData("/speed", payload)
}


async function updateColor() {
  const RVal = document.querySelector("#R-slider").value
  const GVal = document.querySelector("#G-slider").value
  const BVal = document.querySelector("#B-slider").value

  const payload = {
    r: RVal,
    g: GVal,
    b: BVal
  }

  console.log(payload);
}


loadItems();