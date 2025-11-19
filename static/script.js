function printToConsole(text) {
    const outputDiv = document.getElementById('output');
    outputDiv.innerHTML += text + '<br>';
}

function resetConsoleText() {
    const outputDiv = document.getElementById('output');
    outputDiv.innerHTML = "";
}

function loadFile(fileName, event) {
    const file = event.target.files[0];
    if (file) {
        const reader = new FileReader();
        reader.onload = function (e) {
            const contents = e.target.result;
            FS.writeFile(fileName, contents);
        };
        reader.readAsText(file);
    }
}

function visualizeSvg(fileName, svgElement) {
    try {
        const svgContent = FS.readFile(fileName, { encoding: 'utf8' });
        svgElement.innerHTML = svgContent;
    } catch (e) {
        console.error('Error loading file:', e);
    }
}

if (typeof Module === 'undefined') Module = {};
Module.print = function (text) {
    printToConsole(text);
};

/////////////////////////////////////
// AUSLANDER & PARTER
/////////////////////////////////////

const fileInput = document.getElementById('fileInput');

fileInput.addEventListener('change', (event) => {
    loadFile('/input.txt', event);
});

function embed() {
    Module.ccall('compute_orthogonal_drawing', null, [], []);
}

const visualizeButton = document.getElementById('visualize');
const svgOutput = document.getElementById('svgOutput');

visualizeButton.addEventListener('click', () => {
    visualizeSvg('/output.svg', svgOutput);
});