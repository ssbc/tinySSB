// prod/tools.js

"use strict";

function load_prod_list() {
    document.getElementById("lst:prod").innerHTML = '';
    load_prod_item('Kanban', 'img/kanban.svg', 'setScenario("kanban")',
                   'text text text h aghjwd gldfhjs hlgsf hgljksf hgls fdhglf sdhgl hfgskj hls dfhgjl shgjkls hgl sfdhgjk sdfjklg hljs hfgl dfjlsfs');
    load_prod_item('Event Scheduler (dpi24.14)', 'img/schedule.svg', 'setScenario("scheduling")',
                   'text text text h aghjwd gldfhjs hlgsf hgljksf hgls fdhglf sdhgl hfgskj hls dfhgjl shgjkls hgl sfdhgjk sdfjklg hljs hfgl dfjlsfs');
    load_prod_item('Kahoot Quiz (dpi24.15)', 'img/quiz.svg', 'xyz',
                   'text text text h aghjwd gldfhjs hlgsf hgljksf hgls fdhglf sdhgl hfgskj hls dfhgjl shgjkls hgl sfdhgjk sdfjklg hljs hfgl dfjlsfs');
    load_prod_item('Lokens (coming soon)', 'img/hand_and_coins.svg', 'xyz',
                   'see Erick Lavoie: "GOC-Ledger: State-based Conflict-Free Replicated Ledger from Grow-Only Counters", https://arxiv.org/abs/2305.16976');
}

function load_prod_item(title, imageName, cb, descr) {
    var row, item = document.createElement('div'), bg;
    item.setAttribute('style', 'padding: 0px 5px 10px 5px;'); // old JS (SDK 23)
    bg = ' light'; // c[1].forgotten ? ' gray' : ' light';
    row = `<button class="app_icon" style="margin-right: 0.75em; background-color: white;"><img width=35 height=35 src="${imageName}"/>`;
    row += `<button class='prod_item_button light' style='overflow: hidden; width: calc(100% - 4em);' onclick='${cb};'>`;
    row += "<div style='white-space: wrap;'><div style='text-overflow: ellipsis; overflow: hidden;'>" + escapeHTML(title) + "</div>";
    row += "<font size=-2>" + escapeHTML(descr) + "</font></div></button>";
    item.innerHTML = row;
    document.getElementById('lst:prod').appendChild(item);
}

// --- eof
