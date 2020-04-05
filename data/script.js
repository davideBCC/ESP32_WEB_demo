let userScore = 0;
let computerScore = 0;
const userScore_span = document.getElementById("user-score");
const computerScore_span = document.getElementById("comp-score");
const scoreBoard_div = document.querySelector(".score-board");
const result_p = document.querySelector(".result > p");
const rock_div = document.getElementById("r");
const paper_div = document.getElementById("p");
const scissors_div = document.getElementById("s");

function getComputerChoice () {
    const choices = ['r','p','s'];
    return (choices[Math.floor(Math.random() * 3)]);
}
function convertToWord(letter) {
    if (letter === 'r') return "Rock";
    if (letter === 'p') return "Paper";
    return "Scissors"
}
function win(userChoice, computerChoice) {
    console.log("user win");
    userScore++;
    userScore_span.innerHTML = userScore;
    const smallUserWord = "user".fontsize(3).sup();
    const smallCompWord = "comp".fontsize(3).sup();
    result_p.innerHTML = `${convertToWord(userChoice)}${smallUserWord} beats ${convertToWord(computerChoice)}${smallCompWord}. You win! 🔥 `;
    
    const userChoice_div = document.getElementById(userChoice);
    userChoice_div.classList.add('green_glow');
    setTimeout(() => userChoice_div.classList.remove('green_glow'), 300);
}
function lose(userChoice, computerChoice) {
    console.log("user lose");
    computerScore++;
    computerScore_span.innerHTML = computerScore;
    const smallUserWord = "user".fontsize(3).sup();
    const smallCompWord = "comp".fontsize(3).sup();
    result_p.innerHTML = `${convertToWord(userChoice)}${smallUserWord} loses to ${convertToWord(computerChoice)}${smallCompWord}. You lose... 🌵  `;
    
    const userChoice_div = document.getElementById(userChoice);
    userChoice_div.classList.add('red_glow');
    setTimeout(() => userChoice_div.classList.remove('red_glow'), 300);
}
function draw(userChoice, computerChoice) {
    console.log("draw");    
    const smallUserWord = "user".fontsize(3).sup();
    const smallCompWord = "comp".fontsize(3).sup();
    result_p.innerHTML = `${convertToWord(userChoice)}${smallUserWord} equals ${convertToWord(computerChoice)}${smallCompWord}. It's a draw. 🥶`;
    
    const userChoice_div = document.getElementById(userChoice);
    userChoice_div.classList.add('gray_glow');
    setTimeout(() => userChoice_div.classList.remove('gray_glow'), 300);

}

function game(userChoice) {
    computerChoice = getComputerChoice();
    switch(userChoice + computerChoice) {
        case 'pr':
        case 'sp':
        case 'rs':
            win(userChoice, computerChoice);
            break;
        case 'rp':
        case 'ps':
        case 'sr':
            lose(userChoice, computerChoice);
            break;
        default:
            draw(userChoice, computerChoice);
    }
    console.log("userScore = " + userScore);
    console.log("computerScore = " + computerScore);
}


function main() {
    rock_div.addEventListener('click', () => game("r"));

    paper_div.addEventListener('click', () => game("p"));

    scissors_div.addEventListener('click', () => game("s"));
}

main();