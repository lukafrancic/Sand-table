let selectedId = null;
let selectedMeta = null;

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
  
  await fetch("/submit", {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify(payload)
  });

  alert("Data sent!");
}


loadItems();


async function buttonPress(task) {
  const payload = {
    task: task  
  };

  await fetch("/button", {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify(payload)
  });

  console.log(payload)
  alert("Button pressed!");
}


async function home() {
    buttonPress("home");
}


async function start() {
    buttonPress("start");
}


async function stop() {
    buttonPress("stop");
}


async function clearQueue() {
    buttonPress("clear");
}