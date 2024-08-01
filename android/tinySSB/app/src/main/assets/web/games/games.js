// games.js

"use strict";

function load_game_list() {
    document.getElementById("lst:games").innerHTML = '';
    load_game_item('Battleship (dpi24.06)', 'games/dpi24-06-battelship/battleship.svg',
                   'text text text h aghjwd gldfhjs hlgsf hgljksf hgls fdhglf sdhgl hfgskj hls dfhgjl shgjkls hgl sfdhgjk sdfjklg hljs hfgl dfjlsfs');
    load_game_item('Snake (dpi24.07)', 'games/dpi24-07-snake/snake.png',
                   'text text text h aghjwd gldfhjs hlgsf hgljksf hgls fdhglf sdhgl hfgskj hls dfhgjl shgjkls hgl sfdhgjk sdfjklg hljs hfgl dfjlsfs');
    load_game_item('Checkers (dpi24.08)', 'games/dpi24-08-checkers/checkers.svg',
                   'text text text h aghjwd gldfhjs hlgsf hgljksf hgls fdhglf sdhgl hfgskj hls dfhgjl shgjkls hgl sfdhgjk sdfjklg hljs hfgl dfjlsfs');
    load_game_item('Connect4 (dpi24.09)', 'games/dpi24-09-connect4/connect4.png',
                   'text text text h aghjwd gldfhjs hlgsf hgljksf hgls fdhglf sdhgl hfgskj hls dfhgjl shgjkls hgl sfdhgjk sdfjklg hljs hfgl dfjlsfs');
    load_game_item('Blackjack (dpi24.10)', 'games/dpi24-10-blackjack/coins.svg',
                   'crypto tokens based on CRDTs, no mining needed. Ideal for fidelity cards, bartering, recognition tokens in open SW communities, and more.');
    load_game_item('Hangman (dpi24.11)', 'games/dpi24-11-hangman/hangman.svg',
                   'dah dah dah');
}

function load_game_item(title, imageName, descr) {
    var row, item = document.createElement('div'), bg;
    item.setAttribute('style', 'padding: 0px 5px 10px 5px;'); // old JS (SDK 23)
    bg = ' light'; // c[1].forgotten ? ' gray' : ' light';
    row = `<button class="app_icon" style="margin-right: 0.75em; background-color: white;"><img width=35 height=35 src="${imageName}"/>`;
    row += "<button class='prod_item_button light' style='overflow: hidden; width: calc(100% - 4em);' onclick='show_contact_details();'>";
    row += "<div style='white-space: wrap;'><div style='text-overflow: ellipsis; overflow: hidden;'>" + escapeHTML(title) + "</div>";
    row += "<font size=-2>" + escapeHTML(descr) + "</font></div></button>";
    item.innerHTML = row;
    document.getElementById('lst:games').appendChild(item);
}

// --- eof
