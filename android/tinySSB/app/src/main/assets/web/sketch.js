"use strict";

function chat_openSketch() {
    closeOverlay()
    // Create a canvas element
    var canvas = document.createElement('canvas');
    canvas.id = 'sketchCanvas';
    canvas.style.position = 'fixed';
    canvas.style.top = '0';
    canvas.style.left = '0';
    canvas.width = window.innerWidth; // Full screen width
    canvas.height = window.innerHeight; // Full screen height


    canvas.style.backgroundColor = '#ffffff';
    document.body.appendChild(canvas);


    // Create a close button and style it
    var closeButton = document.createElement('button');
    closeButton.id = 'btn:closeSketch';
    closeButton.innerHTML = 'Close';
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
    var submitButton = document.createElement('button');
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
    var colorPalette = document.createElement('div');
    colorPalette.id = 'colorPalette';
    colorPalette.style.position = 'fixed';
    colorPalette.style.bottom = '10px';
    colorPalette.style.left = '45px';
    //document.body.appendChild(colorPalette);

    //Color ring addition
    var colorChoiceButton = document.createElement('img');
    colorChoiceButton.id = 'colorChoiceButton';
    colorChoiceButton.src = 'img/color-wheel.png';
    colorChoiceButton.style.position = 'fixed';
    colorChoiceButton.style.bottom = '10px';
    colorChoiceButton.style.borderRadius = '50%';
    colorChoiceButton.style.left = '10px';
    colorChoiceButton.style.width = '30px';
    colorChoiceButton.style.height = '30px';
    colorChoiceButton.style.display = 'inline-block';
    colorChoiceButton.style.backgroundColor = 'red';
    colorChoiceButton.onclick = setColorPaletteButton;
    document.body.appendChild(colorChoiceButton);

    //Array of colors we want to include
    var colors = ['#000000', '#ff0000', '#00ff00', '#0000ff', '#ffff00', '#ff00ff'];

    // Add color buttons to the color palette
    colors.forEach(function(color) {
      var colorSwatch = document.createElement('div');
      colorSwatch.style.backgroundColor = color;
      colorSwatch.style.width = '20px';
      colorSwatch.style.height = '20px';
      colorSwatch.style.borderRadius = '50%';
      colorSwatch.style.display = 'inline-block';
      colorSwatch.style.marginRight = '5px';
      colorSwatch.style.cursor = 'pointer';
      colorSwatch.onclick = function() {
        setStrokeColor(color);
      };
      colorPalette.appendChild(colorSwatch);
    });

    //Create and style small thickness button
    var changeSmallLine = document.createElement('div');
    changeSmallLine.id = 'changeSmallLine';
    changeSmallLine.style.position = 'fixed';
    changeSmallLine.style.bottom = '10px';
    changeSmallLine.style.right = '50px';
    changeSmallLine.style.width = '10px';
    changeSmallLine.style.height = '10px';
    changeSmallLine.style.display = 'inline-block';
    changeSmallLine.style.backgroundColor = 'black';
    changeSmallLine.onclick =  () => {changeThickness(2);};
    document.body.appendChild(changeSmallLine);

    //Do the same for medium thickness
    var changeMediumLine = document.createElement('div');
    changeMediumLine.id = 'changeMediumLine';
    changeMediumLine.style.position = 'fixed';
    changeMediumLine.style.bottom = '10px';
    changeMediumLine.style.right = '65px';
    changeMediumLine.style.width = '15px';
    changeMediumLine.style.height = '15px';
    changeMediumLine.style.display = 'inline-block';
    changeMediumLine.style.backgroundColor = 'black';
    changeMediumLine.onclick =  () => {changeThickness(5);};
    document.body.appendChild(changeMediumLine);

    //Do the same for large thickness
    var changeLargeLine = document.createElement('div');
    changeLargeLine.id = 'changeLargeLine';
    changeLargeLine.style.position = 'fixed';
    changeLargeLine.style.bottom = '10px';
    changeLargeLine.style.right = '85px';
    changeLargeLine.style.width = '20px';
    changeLargeLine.style.height = '20px';
    changeLargeLine.style.display = 'inline-block';
    changeLargeLine.style.backgroundColor = 'black';
    changeLargeLine.onclick =  () => {changeThickness(10);};
    document.body.appendChild(changeLargeLine);

    //Add an eraser
    var eraserSign = document.createElement('img');
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
    var ctx = canvas.getContext('2d');
    //ctx.fillStyle = "white";
    //ctx.fillRect(0 , 0, canvas.width, canvas.height)
    var currentWidth = 2;
    var strokeColor = '#000000';
    var isDrawing = false;
    var isEraserEnabled = false;
    var lastX = 0;
    var lastY = 0;
    var colChoice = true;
    var currentColor = '#000000'

    canvas.addEventListener('touchstart', startDrawing);
    canvas.addEventListener('touchmove', draw);
    canvas.addEventListener('touchend', endDrawing);
    canvas.addEventListener('touchcancel', endDrawing);

    //Drawing function when user starts drawing (on touchstart)
    function startDrawing(e) {
        e.preventDefault();
        isDrawing = true;
        var rect = e.target.getBoundingClientRect();
        [lastX, lastY] = [e.touches[0].clientX - rect.left, e.touches[0].clientY - rect.top];
    }


    //Function when users move their finger to continue drawing
    function draw(e) {
        if (!isDrawing) return;
        e.preventDefault();
        var rect = e.target.getBoundingClientRect();
        ctx.beginPath();
        ctx.strokeStyle = strokeColor;
        ctx.moveTo(lastX, lastY);
        ctx.lineTo(e.touches[0].clientX - rect.left, e.touches[0].clientY - rect.top);
        ctx.stroke();
        [lastX, lastY] = [e.touches[0].clientX - rect.left, e.touches[0].clientY - rect.top];
    }

    //set isDrawing to false when users remove their finger
    function endDrawing() {
        isDrawing = false;
    }

    //function to decide the drawing color, called by the color buttons above
    function setStrokeColor(color) {
        ctx.globalCompositeOperation = 'source-over';
        strokeColor = color;
        currentColor = color
    }

    //function to either add or remove the color Palette depending on if  it exists
    //Called when the color ring is clicked
    function setColorPaletteButton () {
        if (colChoice == true) {
            document.body.appendChild(colorPalette);
            colChoice = false;
        } else {
            colorPalette.parentNode.removeChild(colorPalette);
            colChoice = true;
        }

    }

    //function to change thickness of the pinsel, called by the thickness buttons above
    function changeThickness(x) {
       ctx.lineWidth = x;
       currentWidth = x;
    }

    //eraser function
    function toggleEraser() {
        isEraserEnabled = !isEraserEnabled;
        if (isEraserEnabled) {
          ctx.globalCompositeOperation = 'destination-out';
          ctx.strokeStyle = "rgba(255,255,255,1)";
          ctx.lineWidth = 30;
          eraserSign.style.border = '1px solid red';
        } else {
          setStrokeColor(currentColor)
          ctx.lineWidth = currentWidth;
          eraserSign.style.border = '';
        }
      }
}

//function called by the close button to end the sketch
function chat_closeSketch() {
  // Remove the canvas element
  var canvas = document.getElementById('sketchCanvas');
  canvas.parentNode.removeChild(canvas);

  // Remove the close button
  var closeButton = document.getElementById('btn:closeSketch');
  closeButton.parentNode.removeChild(closeButton);

  var submitButton = document.getElementById('btn:submitSketch');
  submitButton.parentNode.removeChild(submitButton);

  // Remove the color Choice Button
  var colorChoiceButton = document.getElementById('colorChoiceButton');
  colorChoiceButton.parentNode.removeChild(colorChoiceButton);

  // Remove the eraser sign
  var eraserSign = document.getElementById('eraserSign');
  eraserSign.parentNode.removeChild(eraserSign);

  //Remove the changeSmallLine Button
  var changeSmallLine = document.getElementById('changeSmallLine');
  changeSmallLine.parentNode.removeChild(changeSmallLine);

  //Remove the changeMediumLine Button
  var changeMediumLine = document.getElementById('changeMediumLine');
  changeMediumLine.parentNode.removeChild(changeMediumLine);

  //Remove the changeLargeLine Button
  var changeLargeLine = document.getElementById('changeLargeLine');
  changeLargeLine.parentNode.removeChild(changeLargeLine);

  // Remove the color palette if it exists (is open)
  var colorPalette = document.getElementById('colorPalette');
  if (colorPalette) {
    colorPalette.parentNode.removeChild(colorPalette);
  }
}

//function called by the drawing submit button
//gets the data uri of the drawing in the canvas and sends it to new_drawing_post
function chat_sendDrawing() {
    var drawingUrl = document.getElementById("sketchCanvas").toDataURL('image/png');
    chat_new_drawing_post(drawingUrl);
    chat_closeSketch();
}

function chat_new_drawing_post(s) {
    if (s.length == 0) {
        return;
    }
    //var canvas = document.getElementById('sketchCanvas')
    //var ctx = canvas.getContext('2d');
    //ctx.scale(canvas.width / 10000,  canvas.height / 10000)

    var drawingUrl = document.getElementById('sketchCanvas').toDataURL('image/png');
    // drawingUrl = resizeBase64Img(drawingUrl, canvas.width / 5, canvas.height / 5)

    console.log("NOTCOMPRESSED", drawingUrl.length)

    var data = drawingUrl.split(',')[1];

    // We Convert the data to a Uint8Array
    var byteArray = atob(data)
      .split('')
      .map(function (char) {
        return char.charCodeAt(0);
      });
    var uint8Array = new Uint8Array(byteArray);

    // We Use pako to compress the Uint8Array
    var compressedData = pako.deflate(uint8Array);

    // We Convert the compressed data back to a base64 string
    var compressedBase64 = btoa(String.fromCharCode.apply(null, compressedData));

    // We Create a new data URL with the compressed data
    var shortenedDataURL = 'data:image/png;base64,' + compressedBase64;

    console.log("COMPRESSED", shortenedDataURL.length)

    var recps;
    if (curr_chat == "ALL") {
        recps = "ALL";
        backend("publ:post [] " + btoa(shortenedDataURL) + " null"); //  + recps)
    } else {
        recps = tremola.chats[curr_chat].members.join(' ');
        backend("priv:post [] " + btoa(shortenedDataURL) + " null " + recps);
    }
    closeOverlay();
    setTimeout(function () { // let image rendering (fetching size) take place before we scroll
        var c = document.getElementById('core');
        c.scrollTop = c.scrollHeight;
    }, 100);
}
