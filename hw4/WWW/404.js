var dancingWords = [];
var song;

function DanceSpan(element, x, y) {
    element.position(x, y);

    this.brownian = function() {
        x += random(-6, 6);
        y += random(-6, 6);
        element.position(x, y);
    };
};

function preload() {
    song = loadSound('/Nyan_cat.ogg');
}

function setup() {
    song.loop();

    var texts = selectAll('.text');

    for (var i=0; i<texts.length; i++) {
        var paragraph = texts[i].html();
        var words = paragraph.split(' ');
        for (var j=0; j<words.length; j++) {   
            var spannedWord = createSpan(words[j]);
            var dw = new DanceSpan(spannedWord, random(600), random(200));
            dancingWords.push(dw);
        };   
    };
    
    var img = createImg('/nyan2.gif');
    
}

function draw() {
    for (var i=0; i<dancingWords.length; i++) {
        dancingWords[i].brownian();
    };
}
