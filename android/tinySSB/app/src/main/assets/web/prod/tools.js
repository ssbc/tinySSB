// prod/tools.js

"use strict";

function load_prod_list() {
    document.getElementById("lst:prod").innerHTML = '';
    load_prod_item('Kanban', 'prod/kanban/kanban.svg', 'setScenario("kanban")',
                   'Collaboratively visualize your work items and give participants a view of progress and process, from start to finish.<br><em>Author: Jannick Heisch</em>');
    load_prod_item('Event Scheduler (dpi24.14)', 'prod/dpi24-14-sched/schedule.svg', null /* 'setScenario("scheduling")' */,
                   'Collaboratively find suitable dates by collecting availability of participants.<br><em>Authors: Sascha Schumacher and Jasra Mohamed Yoosuf</em>');
    load_prod_item('Kahoot Quiz (dpi24.15)', 'prod/dpi24-15-kahoot/quiz.svg', null /* 'setScenario("kahoot")' */,
                   'Create and participate in quizzes - a fun way for users to test their knowledge and learn new information.<br><em>Authors: Anoozh Akileswaran, Prabavan Balasubramaniam and Jakob Spiess</em>');
    load_prod_item('Lokens (coming soon)', 'prod/hand_and_coins.svg', null /* 'xyz' */,
                   'see Erick Lavoie: <em>GOC-Ledger: State-based Conflict-Free Replicated Ledger from Grow-Only Counters</em>, <a href="https://arxiv.org/abs/2305.16976">https://arxiv.org/abs/2305.16976</a>');
}

function load_prod_item(title, imageName, fct, descr) {
    var row, item = document.createElement('div'), bg;
    item.setAttribute('style', 'padding: 0px 5px 10px 5px;'); // old JS (SDK 23)
    bg = fct == null ? 'gray' : 'light'
    fct = fct == null ? 'launch_snackbar("tool not imported yet")' : fct
    row = `<button class="app_icon" style="margin-right: 0.75em; background-color: white;"><img width=35 height=35 src="${imageName}"/>`;
    row += `<button class='prod_item_button ${bg}' style='overflow: hidden; width: calc(100% - 4em);' onclick='${fct};'>`;
    row += "<div style='white-space: wrap;'><div style='text-overflow: ellipsis; overflow: hidden;'><strong>" + escapeHTML(title) + "</strong></div>";
    row += descr + "</div></button>"; // "<font size=-2>" + descr + "</font></div></button>";
    item.innerHTML = row;
    document.getElementById('lst:prod').appendChild(item);
}

// --- eof
