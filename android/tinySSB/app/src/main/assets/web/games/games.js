// games.js

"use strict";

function load_game_list() {
    document.getElementById("lst:games").innerHTML = '';
    load_game_item('Battleship (dpi24.06)', 'games/dpi24-06-battleship/battleship.svg', null /*'show_duels()'*/,
                   "Strategy game for two, played on ruled grids on which each player's fleet of warships (randomly generated) are positioned.<br><em>Authors: Mirco Franco and Lars Schneider</em>");
    /* excluded because chrome emulator-only
    load_game_item('Snake (dpi24.07)', 'games/dpi24-07-snake/snake.png', '',
                   'text text text h aghjwd gldfhjs hlgsf hgljksf hgls fdhglf sdhgl hfgskj hls dfhgjl shgjkls hgl sfdhgjk sdfjklg hljs hfgl dfjlsfs');
    */
    /* excluded because it used the old tremola code based instead of tinySSB
    load_game_item('Checkers (dpi24.08)', 'games/dpi24-08-checkers/checkers.svg', '',
                   'text text text h aghjwd gldfhjs hlgsf hgljksf hgls fdhglf sdhgl hfgskj hls dfhgjl shgjkls hgl sfdhgjk sdfjklg hljs hfgl dfjlsfs');
    */
    load_game_item('Connect4 (dpi24.09)', 'games/dpi24-09-connect4/connect4.png', null /* 'setScenario("connect4-game")' */,
                   'Drop a coin in one of the slots - you win if four of your coins form a straight line.<br><em>Authors: Jan Büchele, Luigj Lazri and Jan Walliser</em>');
    load_game_item('Blackjack (dpi24.10)', 'games/dpi24-10-blackjack/coins.svg', null /* '' */,
                   '<em>Authors: Niklas Hasenkopf, Henrik Lümkemann, Roman Ostermiller</em>');
    load_game_item('Hangman (dpi24.11)', 'games/dpi24-11-hangman/hangman.svg', null /* '' */,
                   '<em>Authors: Matyas Bartha, Mike Baumgartner and Alexander Lutsch</em>');
}

function load_game_item(title, imageName, fct, descr) {
    var row, item = document.createElement('div'), bg;
    item.setAttribute('style', 'padding: 0px 5px 10px 5px;'); // old JS (SDK 23)
    bg = fct == null ? 'gray' : 'light'
    fct = fct == null ? 'launch_snackbar("game not imported yet")' : fct
    row = `<button class="app_icon" style="margin-right: 0.75em; background-color: white;"><img width=35 height=35 src="${imageName}"/>`;
    row += `<button class='prod_item_button ${bg}' style='overflow: hidden; width: calc(100% - 4em);' onclick='${fct};'>`;
    row += "<div style='white-space: wrap;'><div style='text-overflow: ellipsis; overflow: hidden;'><strong>" + escapeHTML(title) + "</strong></div>";
    row += descr + "</div></button>";
    item.innerHTML = row;
    document.getElementById('lst:games').appendChild(item);
}

// --- eof
