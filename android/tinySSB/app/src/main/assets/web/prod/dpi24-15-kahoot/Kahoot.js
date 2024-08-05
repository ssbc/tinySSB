//Kahoot.js
//"Questions": [{"qID": '', "Question":'', "Answers": [], "score": 0 }, ...]

var my_QuestionSet = [];
var block_dislike = [];

var Ranks = [];
var current_QuestionSet;
var current_Question;
var current_answers = [];

const Operation_KAH = {
    UPDATE_SCORE: 'update_ranking',
    NEW_QUESTIONSET: 'create_QuestionSet',
    NEW_STATUS_UPDATE: 'new_status',
    NEW_TOTAL_POINTS: 'new_points',
    DISLIKE: 'dislike'

};



//Incoming new event caught

function Kahoot_new_event(e){
    console.log('new kahoot event ' + JSON.stringify(e))
    //'public': ["KAH", SendID, cmdStr[2]].concat(args)
    var SendID = e.public[1];
    var operation = e.public[2];
    var ignore = e.public[3]
    var args = atob(e.public[4]);

    if(!(SendID in tremola.kahoot)){
        tremola.kahoot[SendID] = {
            SendID: SendID,
            QuestionSets: [],
            playerScore: 0,
            ignore: ignore,
            block_dislike: []
        }
    }

    if(operation == Operation_KAH.UPDATE_SCORE){
        update_ranking(SendID, args);

    }else if (operation == Operation_KAH.NEW_QUESTIONSET){
            if(SendID != myId){
                console.log("This is the ID "+SendID);
                create_QuestionSet(SendID, JSON.parse(args));

            }else{
                console.log("ADD TO QUESTIONSET: "+JSON.parse(args));
                addToMyQuestionSet(JSON.parse(args));


            }


    }else if( (operation == Operation_KAH.NEW_STATUS_UPDATE) && (ignore == "false") && (SendID == myId)){
        console.log("Operation_KAH.NEW_STATUS_UPDATE");
        update_Status(args);
    }else if((operation == Operation_KAH.NEW_TOTAL_POINTS) && (ignore == "false") && (SendID == myId)){
        update_new_totalPoints(args);
        console.log(" Operation_KAH.NEW_TOTAL_POINTS");
    }else if(operation == Operation_KAH.DISLIKE){
        new_dislike_event(SendID ,args);


    }



}




function new_dislike_event( SendID ,args){

  let questionSetID = args

    for(let m in tremola.kahoot){
        for(let i = 0; i< tremola.kahoot[m].QuestionSets.length; ++i){
            if(tremola.kahoot[m].QuestionSets[i].QuestionSetID == questionSetID){
              if(myId == SendID){
                    if(!tremola.kahoot[myId].block_dislike.includes(parseInt(tremola.kahoot[m].QuestionSets[i].QuestionSetID))){

                            console.log("BLOCK_DISLIKE:"+block_dislike + "and"+ "MyQuestionSet:"+tremola.kahoot[m].QuestionSets[i].QuestionSetID);
                            tremola.kahoot[m].QuestionSets[i].dislike += 1;
                            checkNumberDislike(m, tremola.kahoot[m].QuestionSets[i] );
                            tremola.kahoot[myId].block_dislike.push(parseInt(questionSetID));
                            listQuestionSet();
                            enter_game();
                            return;

                    }
                    return;

              }
              tremola.kahoot[m].QuestionSets[i].dislike += 1;
              checkNumberDislike(m, tremola.kahoot[m].QuestionSets[i] );
              enter_game();
              return;





            }

        }
    }



}


function checkNumberDislike(playerID, QuestionSet){

    if(QuestionSet.dislike==2){
        tremola.kahoot[playerID].playerScore = tremola.kahoot[playerID].playerScore - 2;



    }





}







function update_Status(args){
   let questionSetID = args.split("$")[1];
   let status_temp = args.split("$")[0];
    for(let m in tremola.kahoot){
        for(let i = 0; i< tremola.kahoot[m].QuestionSets.length; ++i){
            if(tremola.kahoot[m].QuestionSets[i].QuestionSetID == questionSetID){
                    tremola.kahoot[m].QuestionSets[i].status = status_temp;
                    enter_game()
            }

        }
    }



}


function update_new_totalPoints(args){
   let questionSetID = args.split("$")[1];
   let points_temp = args.split("$")[0];
    for(let m in tremola.kahoot){
        for(let i = 0; i< tremola.kahoot[m].QuestionSets.length; ++i){
            if(tremola.kahoot[m].QuestionSets[i].QuestionSetID == questionSetID){
                    tremola.kahoot[m].QuestionSets[i].TotalScore =parseInt(points_temp);
                    enter_game()
            }

        }
    }



}


//Update new rankingList.


function update_ranking(SendID, score){
    tremola.kahoot[SendID].playerScore = parseInt(score);
    UI_ranking_update()

}



//Add new QuestionSet to my QuestionSetList

function addToMyQuestionSet(questionSet){
    console.log("IAMHERE******");
    if(tremola.kahoot[myId].QuestionSets.length!=0){
        for(let i = 0; i< tremola.kahoot[myId].QuestionSets.length; ++i){
            if(tremola.kahoot[myId].QuestionSets[i].QuestionSetID == questionSet.QuestionSetID){
                console.log("IAMHERE******2");
                return;
            }

        }
    }
    console.log("IAMHERE******3");
    my_QuestionSet.push(questionSet);
    console.log("IAMHERE******4");
    tremola.kahoot[myId].QuestionSets.push(questionSet);
    console.log("My QuestionSet: " + tremola.kahoot[myId].QuestionSets);


}






//Send Data to  tremola.backend


function kahoot_send_to_backend(data){
    console.log("Before Backend-message: "+ data['args']);
    var SendID = data.SendID;
    var args = data['args'] != null ? btoa(data['args']) : "null";
    var op = data.cmd;
    var ignore = data.ignore;
    var to_backend = ['kahoot', SendID, op, ignore, args];
    console.log("Backend-message: "+ to_backend);
    backend(to_backend.join(" "));

}



//Update the score and send to backend


function new_score_update(score){
    var data = {
        SendID: myId,
        cmd: Operation_KAH.UPDATE_SCORE,
        args: score,
        ignore: "true"
    }
    kahoot_send_to_backend(data);


}



//Send new created QuestionSet to the backend


function new_QuestionSet(questionSet){
    console.log("NEW_QUESTIONSET:" + questionSet );
    var questionSetJson = JSON.stringify(questionSet);

    var data = {
    SendID: myId,
    cmd: Operation_KAH.NEW_QUESTIONSET,
    args: questionSetJson,
    ignore: "true"
    };

    kahoot_send_to_backend(data);
}


//Store the received QuestionSet.


function create_QuestionSet(SendID, QuestionSet){
    console.log("I am at create_QuestionSet");
    for(let q in tremola.kahoot[SendID].QuestionSets){
        if(q.QuestionSetID == QuestionSet.QuestionSetID){

            return;
        }

    }
    tremola.kahoot[SendID].QuestionSets.push(QuestionSet);
    console.log(tremola.kahoot[SendID].QuestionSets);
    console.log("Current QuestionSetID:"+ QuestionSet.QuestionSetID);
    UI_new_QuestionSet();

}


function new_dislike(questionSetID){
       var data = {
            SendID: myId,
            cmd: Operation_KAH.DISLIKE,
            args: questionSetID,
            ignore: "true"
            };

       kahoot_send_to_backend(data);




}


function new_status(questionSetID, status){
        let arg = status +"$"+ questionSetID;
        var data = {
        SendID: myId,
        cmd: Operation_KAH.NEW_STATUS_UPDATE,
        args: arg,
        ignore: "false"
        };

        kahoot_send_to_backend(data);


}

function new_totalPoints(questionSetID, TotalScore){
        let arg = TotalScore +"$"+ questionSetID;
        console.log(arg);
        var data = {
            SendID: myId,
            cmd: Operation_KAH.NEW_TOTAL_POINTS,
            args: arg,
            ignore: "false"
        };

        kahoot_send_to_backend(data);


}
//Fill up the Rank-List.


function getRanks(){
    Ranks.splice(0, Ranks.length);
    for(let m in tremola.kahoot){
        var player = tremola.kahoot[m];
        Ranks.push({name: player.SendID, score: player.playerScore});
    }

    Ranks.sort((a,b)=> b.score-a.score);
}



//get my Score

function getMyScore(){

    return tremola.kahoot[myId].playerScore;


}



//get my ID

function getmyID(){

    return tremola.kahoot[myId].SendID;


}


function getUserPersonal(){
   if(!(myId in tremola.kahoot)){
       let temp = document.getElementById('MyUsername');
       temp.textContent = tremola.contacts[myId].alias;
       temp = document.getElementById('MyPoints');
       temp.textContent = "Points: "+ 0;
    }else{
           let temp = document.getElementById('MyUsername');
               temp.textContent = tremola.contacts[myId].alias;
               temp = document.getElementById('MyPoints');
               temp.textContent = "Points: "+ tremola.kahoot[myId].playerScore;

    }



}

function load_my_QuestionList() {
     counter = 0;

    new_score_update(0);
    if((myId in tremola.kahoot)){
          for(let i = 0; i<tremola.kahoot[myId].QuestionSets.length;++i){
                my_QuestionSet.push(tremola.kahoot[myId].QuestionSets[i]);


          }
    }

   if(!(myId in tremola.kahoot)){
         tremola.kahoot[myId] = {
                   SendID: myId,
                   QuestionSets: [],
                   playerScore: 0,
                   ignore: "true",
                   block_dislike: []
         }

    }



}


