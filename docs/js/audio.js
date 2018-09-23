/*jslint browser: true*/ /*global  $*/
/* Global variables */

var is_safari = /^((?!chrome|android).)*safari/i.test(navigator.userAgent);

var wavesurfer = WaveSurfer.create({
    container: '#waveform',
    waveColor: '#82b1ff',
    interact: false,
    barWidth: 3,
    progressColor: '#f7f7f7',
    backend: 'MediaElement'
});

var wavesurfer2 = WaveSurfer.create({
    container: '#waveform2',
    waveColor: '#82b1ff',
    interact: false,
    barWidth: 3,
    progressColor: '#f7f7f7',
    backend: 'MediaElement'
});

var options = {
        strings: [""],
        showCursor: false
};

var typedObj = new Typed("#typed", options);
var typedObj2 = new Typed("#typed2", options);
var currentCardNumber = -1;
var currentId = -1;

$(function () {
    'use strict';
    $('#player').hide();
    $('#player2').hide();
});

function playFile() {
    'use strict';
    typedObj.strings = [settings[currentId].song_text];
    typedObj.typeSpeed = settings[currentId].type_speed;
    typedObj.reset();
    wavesurfer.play();
}

function playFile2() {
    'use strict';
    typedObj2.strings = [settings[currentId].song_text];
    typedObj2.typeSpeed = settings[currentId].type_speed;
    typedObj2.reset();
    wavesurfer2.play();
}

function loadFile(cardNumber, id) {
    'use strict';
    currentCardNumber = cardNumber;
    currentId = id;

    stopFile2();
    wavesurfer.stop();
    wavesurfer.empty();
    $('#playBtn').prop('disabled', true);
    $('#stopBtn').prop('disabled', true);
    typedObj.reset(false);

    $('#player').show(500, function () {
        wavesurfer.load('audio/' + settings[currentId].file);
    });

    wavesurfer.on('ready', function () {
        $('#playBtn').prop('disabled', false);
        $('#stopBtn').prop('disabled', false);
        if (!(is_safari)) { // If not Safari improve usability by auto-playing
            playFile();
        }
    });
}

function loadFile2(cardNumber, id) {
    'use strict';
    currentCardNumber = cardNumber;
    currentId = id;

    stopFile();
    wavesurfer2.stop();
    wavesurfer2.empty();
    $('#playBtn2').prop('disabled', true);
    $('#stopBtn2').prop('disabled', true);
    typedObj2.reset(false);

    $('#player2').show(500, function () {
        wavesurfer2.load('audio/' + settings[currentId].file);
    });

    wavesurfer2.on('ready', function () {
        $('#playBtn2').prop('disabled', false);
        $('#stopBtn2').prop('disabled', false);
        if (!(is_safari)) { // If not Safari improve usability by auto-playing
            playFile2();
        }
    });
}

function stopFile() {
    'use strict';
    wavesurfer.stop();
    $('#playBtn').prop('disabled', true);
    $('#stopBtn').prop('disabled', true);
    wavesurfer.empty();
    wavesurfer.unAll();
    typedObj.reset(false);
    //$('#spectrums').hide(500);
    $('#player').hide(500);
}

function stopFile2() {
    'use strict';
    wavesurfer2.stop();
    $('#playBtn2').prop('disabled', true);
    $('#stopBtn2').prop('disabled', true);
    wavesurfer2.empty();
    wavesurfer2.unAll();
    typedObj2.reset(false);
    //$('#spectrums').hide(500);
    $('#player2').hide(500);
}
