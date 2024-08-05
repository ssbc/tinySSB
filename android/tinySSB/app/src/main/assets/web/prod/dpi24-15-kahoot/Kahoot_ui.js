//Kahoot_ui.js


//Kahoot_ui.js
function btn_create_quiz() {
    console.log("I am here in btn_create_quiz()");
    document.getElementById('kahoot-overlay').style.display = 'none';
    document.getElementById('quiz-create-game-overlay').style.display = 'flex';
}

function btn_new_question_set() {
    document.getElementById('quiz-create-game-overlay').style.display = 'none';
    document.getElementById('new-question-set-overlay').style.display = 'flex';
    addQuestion(); // Start with one question
}

function btn_new_question_set_cancel() {
    document.getElementById('new-question-set-overlay').style.display = 'none';
    document.getElementById('quiz-create-game-overlay').style.display = 'flex';
}

function addQuestion() {
    const questionsContainer = document.getElementById('questions-container');
    const questionCount = questionsContainer.children.length;

    if (questionCount >= 8) return; // Limit to 8 questions

    const questionDiv = document.createElement('div');
    questionDiv.classList.add('question');

    let questionHTML = `
      <div>
        <label>Question ${questionCount + 1}:</label>
        <input type="text" name="question${questionCount}" required>
      </div>
    `;

    for (let i = 0; i < 6; i++) {
        questionHTML += `
          <div class="answer">
            <label>Answer ${i + 1}:</label>
            <input type="text" name="question${questionCount}-answer${i}" required>
            <label>Points:</label>
            <input type="number" name="question${questionCount}-points${i}" required>
          </div>
        `;
    }

    questionDiv.innerHTML = questionHTML;
    questionsContainer.appendChild(questionDiv);
}

function submitQuestionSet(event) {
    event.preventDefault();
    const formData = new FormData(event.target);
    const questions = [];

    for (let i = 0; i < 8; i++) {
        if (!formData.has(`question${i}`)) break;

        const question = {
            text: formData.get(`question${i}`),
            answers: []
        };

        for (let j = 0; j < 6; j++) {
            if (!formData.has(`question${i}-answer${j}`)) break;
            question.answers.push({
                text: formData.get(`question${i}-answer${j}`),
                points: parseInt(formData.get(`question${i}-points${j}`), 10)
            });
        }

        questions.push(question);
    }

    console.log('Submitted Question Set:', questions);
    // TODO: Store questions and answers depending on backend implementation
}

function btn_create_quiz_cancel() {
    document.getElementById('quiz-create-game-overlay').style.display = 'none';
    document.getElementById('kahoot-overlay').style.display = 'flex';
}

function btn_list_question_set() {
    //Implementation of listing all question sets, depending on backend implementation
    listQuestionSet();
}




//JAKOBs stuff

function handleRanking1() {
    // Remove an element

    /*
    var elementToRemove = document.getElementById('user-scores');
    if (elementToRemove) {
        elementToRemove.remove();
    }
    */


    // Show an element

    var elementToShow = document.getElementById('user-scores');
    if (elementToShow) {
        elementToShow.classList.remove('hidden');
    }

    //document.getElementById('user-scores').style.display = 'flex';

}

//handleEnterGame
function handleEnterGame() {

}

//handle createGame
function handleCreateGame() {

}



//added
function jakob_btnBridge(e) {
    var e = e.id, m = '';
    if (['btn:chats', 'btn:posts', 'btn:contacts', 'btn:connex', 'btn:kanban', 'btn:kahoot'].indexOf(e) >= 0) {
        jakob_setScenario(e.substring(4));
    }
}

function jakob_setScenario(s) {
    // console.log('setScenario ' + s)
    closeOverlay();
    var lst = ['div:qr', 'core', 'lst:kahoot', 'div:footer', 'plus'];
    if (lst) {
        // if (s != 'posts' && curr_scenario != "members" && curr_scenario != 'posts') {
        if (['chats', 'contacts', 'connex', 'kanban', 'kahoot'].indexOf(curr_scenario) >= 0) {
            var cl = document.getElementById('btn:' + curr_scenario).classList;
            cl.toggle('active', false);
            cl.toggle('passive', true);
        }
        // console.log(' l: ' + lst)
        display_or_not.forEach(function (d) {
            // console.log(' l+' + d);
            if (lst.indexOf(d) < 0) {
                document.getElementById(d).style.display = 'none';
            } else {
                document.getElementById(d).style.display = null;
                // console.log(' l=' + d);
            }
        })
        // console.log('s: ' + s)
        if (s != "board") {
            document.getElementById('tremolaTitle').style.position = null;
        }

        if (s == "posts" || s == "settings" || s == "board") {
            document.getElementById('tremolaTitle').style.display = 'none';
            document.getElementById('conversationTitle').style.display = null;
            // document.getElementById('plus').style.display = 'none';
        } else {
            document.getElementById('tremolaTitle').style.display = null;
            // if (s == "connex") { /* document.getElementById('plus').style.display = 'none'; */}
            // else { /* document.getElementById('plus').style.display = null; */}
            document.getElementById('conversationTitle').style.display = 'none';
        }
        if (lst.indexOf('div:qr') >= 0) {
            prev_scenario = s;
        }
        curr_scenario = s;
        if (['chats', 'contacts', 'connex', 'kanban', 'kahoot'].indexOf(curr_scenario) >= 0) {
            var cl = document.getElementById('btn:' + curr_scenario).classList;
            cl.toggle('active', true);
            cl.toggle('passive', false);
        }
        if (s == 'board')
            document.getElementById('core').style.height = 'calc(100% - 60px)';
        else
            document.getElementById('core').style.height = 'calc(100% - 118px)';

        if (s == 'kanban') {
            var personalBoardAlreadyExists = false
            for (var b in tremola.board) {
                var board = tremola.board[b]
                if (board.flags.indexOf(FLAG.PERSONAL) >= 0 && board.members.length == 1 && board.members[0] == myId) {
                    personalBoardAlreadyExists = true
                    break
                }
            }
            if(!personalBoardAlreadyExists && display_create_personal_board) {
                menu_create_personal_board()
            }
        }
        // Hide kahoot-create-quiz-overlay when switching scenarios
                document.getElementById('quiz-master-title').style.display = 'none';
                // Hide bottom buttons
                document.getElementById('kahoot-buttons').style.display = 'none';
                // Hide user scores table
                document.getElementById('user-scores').style.display = 'none';
                if (s == 'kahoot') {
                    console.log('Kahoot scenario activated');
                    // Any additional initialization logic for Kahoot can go here
                    //document.getElementById('lst:kahoot').style.display = 'block';
                    document.getElementById('quiz-master-title').style.display = 'block';
                    document.getElementById('kahoot-buttons').style.display = 'block';
                    document.getElementById('user-scores').style.display = 'block';
                }
            }
        }

