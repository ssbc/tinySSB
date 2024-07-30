// sketch.js

"use strict";

/* storage format

  IMG := [VERSION, BBOX, BLOCK*]
  VERSION := INT
  BBOX := ['s', INT, INT] % width and height of bounding box, at beginning of IMG
  BLOCK := PATH | COL | WID
  PATH := ['p', INT* ]    % first two ints: X/Y of absolute path start, followed by DX/DY pairs
  COL := ['c', INT]       % color index
  WID := ['w', INT]       % width index
*/
// the maximal dimensions of the sketches in px.
const SKETCH_MAX_HEIGHT = 500
const SKETCH_MAX_WIDTH = 500

const SKETCH_SIZE_UPDATE_INTERVAL = 5000

var sketch = {
  currSize: 0
};
var sketch_size_update_timer = null; // reference to size update interval

function chat_openSketch() {
    closeOverlay()
    // Create a canvas element
    let canvas = document.createElement('canvas');
    canvas.id = 'sketchCanvas';
    canvas.style.position = 'fixed';
    canvas.style.top = '0';
    canvas.style.left = '0';
    canvas.width = window.innerWidth; // Full screen width
    canvas.height = window.innerHeight; // Full screen height

    canvas.style.backgroundColor = '#ffffff';
    document.body.appendChild(canvas);

    let sizeDiv = document.createElement('div')
    sizeDiv.id = 'div:sketch_size'
    sizeDiv.style.position = 'fixed';
    sizeDiv.style.left = '10px';
    sizeDiv.style.top = '18px';
    sizeDiv.innerHTML = 'Size: '

    async function sketch_updateSize() {
        let new_size = await sketch_get_current_size()
        if (new_size == sketch.currSize) {
            return
        }
        sketch.currSize = new_size
        let sizeKB = new_size / 1000
        sizeDiv.innerHTML = 'Size: ' + sizeKB + ' kB'
    }

    sketch_updateSize()
    sketch_size_update_timer = setInterval(async () => {
                                                await sketch_updateSize()
                                            }, SKETCH_SIZE_UPDATE_INTERVAL);
    document.body.appendChild(sizeDiv)

    // Create a close button and style it
    let closeButton = document.createElement('button');
    closeButton.id = 'btn:closeSketch';
    closeButton.innerHTML = 'Cancel';
    closeButton.style.position = 'fixed';
    closeButton.style.top = '10px';
    closeButton.style.right = '10px';
    closeButton.style.padding = '10px';
    closeButton.style.backgroundColor = '#ff0000';
    closeButton.style.color = '#ffffff';
    closeButton.style.border = 'none';
    closeButton.style.borderRadius = '5px';
    closeButton.style.cursor = 'pointer';
    closeButton.onclick = chat_closeSketch;
    document.body.appendChild(closeButton);

    // Do the same with a submit button
    let submitButton = document.createElement('button');
    submitButton.id = 'btn:submitSketch';
    submitButton.innerHTML = 'Submit';
    submitButton.style.position = 'fixed';
    submitButton.style.top = '10px';
    submitButton.style.right = '100px';
    submitButton.style.padding = '10px';
    submitButton.style.backgroundColor = '#008000';
    submitButton.style.color = '#ffffff';
    submitButton.style.border = 'none';
    submitButton.style.borderRadius = '5px';
    submitButton.style.cursor = 'pointer';
    submitButton.onclick = chat_sendDrawing;
    document.body.appendChild(submitButton);

    //Create a color Palette, add it only with setColorPaletteButton function
    let colorPalette = document.createElement('div');
    colorPalette.id = 'colorPalette';
    colorPalette.style.position = 'fixed';
    colorPalette.style.bottom = '10px';
    colorPalette.style.left = '45px'; // 70px';
    //document.body.appendChild(colorPalette);

    /*
    let undoButton = document.createElement('img');
    undoButton.id = 'undoButton';
    undoButton.src = 'img/undo.png';
    undoButton.style.position = 'fixed';
    undoButton.style.bottom = '10px';
    // undoButton.style.borderRadius = '50%';
    undoButton.style.left = '5px';
    undoButton.style.width = '25px';
    undoButton.style.height = '30px';
    undoButton.style.display = 'inline-block';
    undoButton.style.backgroundColor = 'lightgrey';
    undoButton.onclick = undoButtonCallback;
    document.body.appendChild(undoButton);
    */

    //Color ring addition
    let colorChoiceButton = document.createElement('img');
    colorChoiceButton.id = 'colorChoiceButton';
    colorChoiceButton.src = 'img/color-wheel.png';
    colorChoiceButton.style.position = 'fixed';
    colorChoiceButton.style.bottom = '10px';
    colorChoiceButton.style.borderRadius = '50%';
    colorChoiceButton.style.left = '10px'; // '35px';
    colorChoiceButton.style.width = '30px';
    colorChoiceButton.style.height = '30px';
    colorChoiceButton.style.display = 'inline-block';
    colorChoiceButton.style.backgroundColor = 'red';
    colorChoiceButton.onclick = setColorPaletteButton;
    document.body.appendChild(colorChoiceButton);

    //Array of colors we want to include
    var sketch_colors = ['#000000', '#ff0000', '#00ff00', '#0000ff', '#ffff00', '#ff00ff'];

    // Add color buttons to the color palette
    Array.from(Array(sketch_colors.length).keys()).forEach(function(ndx) {
      let colorSwatch = document.createElement('div');
      colorSwatch.style.backgroundColor = sketch_colors[ndx];
      colorSwatch.style.width = '20px';
      colorSwatch.style.height = '20px';
      colorSwatch.style.borderRadius = '50%';
      colorSwatch.style.display = 'inline-block';
      colorSwatch.style.marginRight = '5px';
      colorSwatch.style.cursor = 'pointer';
      colorSwatch.onclick = function() {
        setStrokeColor(ndx);
      };
      colorPalette.appendChild(colorSwatch);
    });

    //Create and style small thickness button
    let changeSmallLine = document.createElement('div');
    changeSmallLine.id = 'changeSmallLine';
    changeSmallLine.style.position = 'fixed';
    changeSmallLine.style.bottom = '10px';
    changeSmallLine.style.right = '40px';
    changeSmallLine.style.width = '10px';
    changeSmallLine.style.height = '10px';
    changeSmallLine.style.display = 'inline-block';
    changeSmallLine.style.backgroundColor = 'black';
    changeSmallLine.style.border = '1px solid red'
    changeSmallLine.onclick =  () => {changeThickness(0);};
    document.body.appendChild(changeSmallLine);

    //Do the same for medium thickness
    let changeMediumLine = document.createElement('div');
    changeMediumLine.id = 'changeMediumLine';
    changeMediumLine.style.position = 'fixed';
    changeMediumLine.style.bottom = '10px';
    changeMediumLine.style.right = '55px';
    changeMediumLine.style.width = '15px';
    changeMediumLine.style.height = '15px';
    changeMediumLine.style.display = 'inline-block';
    changeMediumLine.style.backgroundColor = 'black';
    changeMediumLine.onclick =  () => {changeThickness(1);};
    document.body.appendChild(changeMediumLine);

    //Do the same for large thickness
    let changeLargeLine = document.createElement('div');
    changeLargeLine.id = 'changeLargeLine';
    changeLargeLine.style.position = 'fixed';
    changeLargeLine.style.bottom = '10px';
    changeLargeLine.style.right = '75px';
    changeLargeLine.style.width = '20px';
    changeLargeLine.style.height = '20px';
    changeLargeLine.style.display = 'inline-block';
    changeLargeLine.style.backgroundColor = 'black';
    changeLargeLine.onclick =  () => {changeThickness(2);};
    document.body.appendChild(changeLargeLine);

    //Add an eraser
    let eraserSign = document.createElement('img');
    eraserSign.id = 'eraserSign';
    eraserSign.src = 'img/eraser.png';
    eraserSign.style.position = 'fixed';
    eraserSign.style.bottom = '10px';
    eraserSign.style.right = '10px';
    eraserSign.style.width = '20px';
    eraserSign.style.height = '20px';
    eraserSign.style.cursor = 'pointer';
    eraserSign.onclick = toggleEraser;
    document.body.appendChild(eraserSign);

    //get the context of the canvas and set initial drawing settings
    sketch.ctx = canvas.getContext('2d');
    sketch.ctx.fillStyle = "white";
    sketch.ctx.fillRect(0 , 0, canvas.width, canvas.height);
    sketch.currSize = 0;
    sketch.currentWidNdx = 0;
    sketch.strokeColor = '#000000';
    sketch.isDrawing = false;
    sketch.isEraserEnabled = false;
    sketch.lastX = 0;
    sketch.lastY = 0;
    sketch.colChoice = true;
    sketch.currentColNdx = 0;
    sketch.svg = [1,['s',canvas.width,canvas.height]]

    canvas.addEventListener('touchstart', startDrawing);
    canvas.addEventListener('touchmove', draw);
    canvas.addEventListener('touchend', endDrawing);
    canvas.addEventListener('touchcancel', endDrawing);

    //Drawing function when user starts drawing (on touchstart)
    function startDrawing(e) {
        e.preventDefault();
        sketch.isDrawing = true;
        let rect = e.target.getBoundingClientRect();
        [sketch.lastX, sketch.lastY] = [Math.round(e.touches[0].clientX - rect.left),
                                        Math.round(e.touches[0].clientY - rect.top)];
        sketch.svg.push(['p',sketch.lastX,sketch.lastY])
    }


    //Function when users move their finger to continue drawing
    function draw(e) {
        if (!sketch.isDrawing) return;
        e.preventDefault();
        let rect = e.target.getBoundingClientRect();
        let ctx_ = sketch.ctx;
        ctx_.beginPath();
        ctx_.strokeStyle = sketch.strokeColor;
        ctx_.moveTo(sketch.lastX, sketch.lastY);
        let x = Math.round(e.touches[0].clientX - rect.left);
        let y = Math.round(e.touches[0].clientY - rect.top);
        ctx_.lineTo(x, y);
        ctx_.stroke();
        let path = sketch.svg[sketch.svg.length - 1];
        path.push(x - sketch.lastX);
        path.push(y - sketch.lastY);
        [sketch.lastX, sketch.lastY] = [x, y];
    }

    //set isDrawing to false when users remove their finger
    function endDrawing() {
        sketch.isDrawing = false;
    }

    //function to decide the drawing color, called by the color buttons above
    function setStrokeColor(ndx) {
        sketch.ctx.globalCompositeOperation = 'source-over';
        sketch.strokeColor = sketch_colors[ndx];
        sketch.currentColNdx = ndx;
        sketch.svg.push(['c', ndx]);
    }

    //function to either add or remove the color Palette depending on if  it exists
    //Called when the color ring is clicked
    function setColorPaletteButton () {
        if (sketch.colChoice == true) {
            document.body.appendChild(colorPalette);
            sketch.colChoice = false;
        } else {
            colorPalette.parentNode.removeChild(colorPalette);
            sketch.colChoice = true;
        }
    }

    function undoButtonCallback () {
        console.log("undo")
    }

    // function to change thickness of the pinsel, called by the thickness buttons above
    function changeThickness(ndx) {
       if (sketch.currentWidNdx == ndx)
         return
       let small = document.getElementById("changeSmallLine")
       let medium = document.getElementById("changeMediumLine")
       let large = document.getElementById("changeLargeLine")

       small.style.border = ""
       medium.style.border = ""
       large.style.border = ""

       switch (ndx) {
         case 0:
            small.style.border = '1px solid red';
            sketch.ctx.lineWidth = 2;
            break
         case 1:
            medium.style.border = '1px solid red'
            sketch.ctx.lineWidth = 5;
            break
         case 2:
            large.style.border = '1px solid red'
            sketch.ctx.lineWidth = 10;
            break
       }
       sketch.currentWidNdx = ndx;
       sketch.svg.push(['w', ndx]);
    }

    //eraser function
    function toggleEraser() {
        sketch.isEraserEnabled = !sketch.isEraserEnabled;
        if (sketch.isEraserEnabled) {
          sketch.ctx.globalCompositeOperation = 'destination-out';
          sketch.ctx.strokeStyle = "rgba(255,255,255,1)";
          sketch.ctx.lineWidth = 30;
          eraserSign.style.border = '1px solid red';
          sketch.svg.push(['w', 3]);
          sketch.svg.push(['c', -1])
        } else {
          setStrokeColor(sketch.currentColNdx)
          let w = sketch.currentWidNdx;
          sketch.currentWidNdx = -1; // force update of width
          changeThickness(w);
          eraserSign.style.border = '';
        }
      }
}

//function called by the close button to end the sketch
function chat_closeSketch() {
  if (sketch_size_update_timer) {
    clearInterval(sketch_size_update_timer) // stop updating size
    sketch_size_update_timer = null
  }

  // Remove the canvas element
  let canvas = document.getElementById('sketchCanvas');
  canvas.parentNode.removeChild(canvas);

  let sizeDiv = document.getElementById('div:sketch_size')
  sizeDiv.parentNode.removeChild(sizeDiv)

  // Remove the close button
  let closeButton = document.getElementById('btn:closeSketch');
  closeButton.parentNode.removeChild(closeButton);

  let submitButton = document.getElementById('btn:submitSketch');
  submitButton.parentNode.removeChild(submitButton);

  // Remove the color Choice Button
  let colorChoiceButton = document.getElementById('colorChoiceButton');
  colorChoiceButton.parentNode.removeChild(colorChoiceButton);

  // Remove the eraser sign
  let eraserSign = document.getElementById('eraserSign');
  eraserSign.parentNode.removeChild(eraserSign);

  //Remove the changeSmallLine Button
  let changeSmallLine = document.getElementById('changeSmallLine');
  changeSmallLine.parentNode.removeChild(changeSmallLine);

  //Remove the changeMediumLine Button
  let changeMediumLine = document.getElementById('changeMediumLine');
  changeMediumLine.parentNode.removeChild(changeMediumLine);

  //Remove the changeLargeLine Button
  let changeLargeLine = document.getElementById('changeLargeLine');
  changeLargeLine.parentNode.removeChild(changeLargeLine);

  // Remove the color palette if it exists (is open)
  let colorPalette = document.getElementById('colorPalette');
  if (colorPalette) {
    colorPalette.parentNode.removeChild(colorPalette);
  }

  // Remove the undo button if it exists (is open)
  let undoButton = document.getElementById('undoButton');
  if (undoButton) {
      undoButton.parentNode.removeChild(undoButton);
  }
}

function sketch_reduceResolution(base64String, reductionFactor) {
    return new Promise((resolve, reject) => {
        const img = new Image();
        img.src = base64String;

        // Wait for the image to load
        img.onload = function () {
          let canvas = document.createElement("canvas");

          const reducedWidth = img.width / reductionFactor;
          const reducedHeight = img.height / reductionFactor;
          canvas.width = reducedWidth;
          canvas.height = reducedHeight;

          const ctx_ = canvas.getContext("2d");
          ctx_.drawImage(img, 0, 0, reducedWidth, reducedHeight);
          const resultBase64String = canvas.toDataURL("image/png");
          resolve(resultBase64String);
        };

        img.onerror = function (){
          reject(new Error("Sketch - Failed to reduce resolution"));
        };
      });
}

// returns the size of the base64 string that represents the sketch
async function sketch_get_current_size() {
    let sketch_ = await sketch_getImage()
    return sketch_.length
}

// return the current sketch as a base64 string (including the preceding data type descriptor)
async function sketch_getImage() {
    // console.log('getImage', JSON.stringify(sketch.svg));
    const buf = new ArrayBuffer(bipf_encodingLength(sketch.svg))
    const e = bipf_encode(sketch.svg, buf, 0)
    // console.log('  bipf:', JSON.stringify(new Uint8Array(buf)))
    // console.log('       ', bipf_encodingLength(sketch.svg), btoa(new Uint8Array(buf)))

    /*
    let canvas = document.getElementById("sketchCanvas")
    sketch.drawingUrl = canvas.toDataURL('image/png');
    let reductionFactor = Math.max(canvas.width / SKETCH_MAX_WIDTH, canvas.height / SKETCH_MAX_HEIGHT)
    if (reductionFactor > 1) {
        sketch.drawingUrl = await sketch_reduceResolution(sketch.drawingUrl, reductionFactor)
    }

    let data = sketch.drawingUrl.split(',')[1];

    // We Convert the data to a Uint8Array
    let byteArray = atob(data)
      .split('')
      .map(function (char) {
        return char.charCodeAt(0);
      });
    let uint8Array = new Uint8Array(byteArray);

    // We Use pako to compress the Uint8Array
    let compressedData = pako.deflate(uint8Array);

    // We Convert the compressed data back to a base64 string
    let compressedBase64 = btoa(String.fromCharCode.apply(null, compressedData));

    // We Create a new data URL with the compressed data
    let shortenedDataURL = 'data:image/png;base64,' + compressedBase64;
    */
    // let shortenedDataURL = 'data:image/svg+bipf;base64,' + btoa(new Uint8Array(buf));

    var binary = '';
    var bytes = new Uint8Array( buf );
    var len = bytes.byteLength;
    for (var i = 0; i < len; i++) {
       binary += String.fromCharCode( bytes[ i ] );
    }
    let shortenedDataURL = 'data:image/svg+bipf;base64,' + btoa(binary);

    return shortenedDataURL
}

//function called by the drawing submit button
async function chat_sendDrawing() {
    let img = await sketch_getImage()
    if (img.length == 0) {
            return;
    }

    // send to backend
    let recps;
    if (curr_chat == "ALL") {
        recps = "ALL";
        backend("publ:post [] " + btoa(img) + " null"); //  + recps)
    } else {
        recps = tremola.chats[curr_chat].members.join(' ');
        backend("priv:post [] " + btoa(img) + " null " + recps);
    }
    closeOverlay();
    setTimeout(function () { // let image rendering (fetching size) take place before we scroll
        let c = document.getElementById('core');
        c.scrollTop = c.scrollHeight;
    }, 100);

    // close sketch
    chat_closeSketch();
}
