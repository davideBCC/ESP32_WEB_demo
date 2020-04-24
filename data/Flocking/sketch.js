let boid;
let world;
let boids = [];
let predators = [];
let cnt = 0;
let fps = 60;
function setup() {
  //createCanvas(1280,1024);
  createCanvas(windowWidth, windowHeight);
 
  for (i = 0; i < 100; i++) {
    boids.push(new Boid(createVector(random(100,width-100),random(100,height-100))));
  }
  for (i = 0; i < 1; i++) {
      predators.push(new Boid(createVector(random(100,width-100),random(100,height-100)), color(255,0,0)));
      predators[i].maxVel = 4.1;
      predators[i].influenceRadius = 250;
  }

  world = new World();
  //stroke('purple'); // Change the color

  alignSlider = createSlider(0, 100, 10);
  alignSlider.position(20, 20);
  cohesSlider = createSlider(1, 100, 10);
  cohesSlider.position(20, 50);
  separSlider = createSlider(0, 100, 1);
  separSlider.position(20, 80);
  

}

function draw() {
  const align = alignSlider.value()/10.0;
  const cohes = cohesSlider.value()/10.0;
  const separ = separSlider.value()/10.0;

  // background(220);
  world.draw();

  stroke(0);
  text('alignment ' + align, alignSlider.x * 2 + alignSlider.width, 30);
  text('cohesion ' + cohes, cohesSlider.x * 2 + cohesSlider.width, 60);
  text('separation: ' + separ, separSlider.x * 2 + separSlider.width, 90);

  
  boids.forEach(boid => {
    //boid.external_acc = world.acceleration(boid.position);
    boid.align = align;
    boid.cohes = cohes;
    boid.separ = separ;
    boid.flocking(boids);
    boid.escape(predators);
  });
  boids.forEach(boid => {
    boid.run();
    boid.draw();
  });
  predators.forEach(pred => {
    //pred.external_acc = world.acceleration(pred.position);
    pred.hunting(boids);
    pred.run();
    pred.draw();
  });
  // Draw FPS (rounded to 2 decimal places) at the bottom left of the screen
  cnt++;
  if(cnt > 15) {
    cnt = 0;
    fps = frameRate();    
  }
  fill(255);
  stroke(0);
  text("FPS: " + fps.toFixed(2), 10, height - 10);

}