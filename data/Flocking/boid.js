class Boid {
    height2 = 8;
    base2 = 6;
    minVel = 1;
    maxVel = 4;
    influenceRadius = 60;
    maxAcc = 0.05;
    sColor = color(0,0,0);
    wallThr = 50;

    align = 1;
    cohes = 1;
    separ = 1;

    constructor(pos,fill) {
        this.position = pos.copy();
        this.velocity = p5.Vector.mult(p5.Vector.random2D(),random(this.minVel, this.maxVel));
        this.external_acc = createVector();
        this.internal_acc = createVector();
        if(!fill) {
            this.fillColor = color(255);
        }
        else {
            this.fillColor = fill;
        }
    }

    draw() {
        push();
        translate(this.position);
        rotate(this.velocity.heading());
        stroke(this.sColor);
        fill(this.fillColor);
        //fill(map(this.velocity.mag(),this.minVel,this.maxVel,200,255));
        triangle(-this.height2, -this.base2, this.height2, 0, -this.height2, this.base2);
        pop();
    }

    flocking(boids) {
        this.internal_acc.x = 0;
        this.internal_acc.y = 0;
        let steering_align = createVector();
        let steering_separate = createVector();
        let n_neighbours = 0;
        let centroid = createVector();
        for (let other of boids) {
            if (other != this) { //proceed only if other boid is not the same as this
                let dist = this.position.dist(other.position);
                if (dist < this.influenceRadius)  { //identify if other boid is in the surrounding
                    steering_align.add(other.velocity);

                    centroid.add(other.position);

                    let diff = p5.Vector.sub(this.position, other.position);
                    diff.div((dist * dist)/10 + 0.00001);
                    steering_separate.add(diff);

                    n_neighbours++;
                }                
            }
        }
        if(n_neighbours != 0) {
            steering_align.div(n_neighbours);
            steering_align.setMag(this.velocity.mag());
            let alignAcc =  p5.Vector.sub(steering_align, this.velocity).limit(this.maxAcc*this.align);
            this.internal_acc.add(alignAcc);

            centroid.add(this.position);
            centroid.div(n_neighbours +1 );
            let centrAcc = p5.Vector.sub(centroid, this.position ); 
            centrAcc.div(this.influenceRadius / (this.maxAcc * this.cohes)); //vary linearly the acceleration from 0 (centroid) to 1 (edge of sphere of influence)
            this.internal_acc.add(centrAcc);

            steering_separate.div(n_neighbours).limit(this.maxAcc * this.separ);
            //steering_separate.setMag(this.velocity.mag());
            //let separateAcc =  p5.Vector.sub(steering_separate, this.velocity).limit(this.maxAcc);
            this.internal_acc.add(steering_separate);
        }
    }

    hunting(boids) {
        this.internal_acc.x = 0;
        this.internal_acc.y = 0;
        let target = null;
        let targetFound = false;
        let bestTargetDist = this.influenceRadius;
        for (let other of boids) {
            if (other != this) { //proceed only if other boid is not the same as this
                let dist = this.position.dist(other.position);
                if (dist < bestTargetDist)  { //identify if other boid is in the surrounding
                    if(dist > 5) { //target pray
                        target = other;
                        bestTargetDist = dist;
                        targetFound = true;
                    }
                    else {
                        this.velocity.setMag(this.minVel);
                        boids.splice(boids.indexOf(other),1);
                        // other.position = createVector(width/2,height/2);
                        // other.velocity = p5.Vector.random2D();
                        // other.velocity.setMag(other.minVel);
                        // other.fillColor = color(127);
                        return; //target eaten
                    }
                }                
            }
        }
        if(targetFound) {

            let attackAcc = p5.Vector.sub(target.position, this.position ).limit(this.maxAcc); 
            this.internal_acc.add(attackAcc);
        }
    }

    escape(boids) {
        
        let steering_separate = createVector();
        let n_neighbours = 0;
        for (let other of boids) {
            if (other != this) { //proceed only if other boid is not the same as this
                let dist = this.position.dist(other.position);
                if (dist < this.influenceRadius)  { //identify if other boid is in the surrounding

                    let diff = p5.Vector.sub(this.position, other.position);
                    diff.div((dist * dist) + 0.00001);
                    steering_separate.add(diff);

                    n_neighbours++;
                }                
            }
        }
        if(n_neighbours != 0) {
            this.internal_acc.x = 0;
            this.internal_acc.y = 0;
            steering_separate.div(n_neighbours).limit(this.maxAcc*3);
            this.internal_acc.add(steering_separate);
        }
    }

    run() {
        this.position.add(this.velocity);
        this.velocity.add(this.internal_acc);
        this.velocity.add(this.external_acc);
        this.sColor = color(0,0,0);

        this.internal_acc = createVector(0,0);
        let vel = this.velocity.mag();
        if(vel < this.minVel) {
            this.internal_acc = this.velocity.copy();
            this.internal_acc.setMag(this.maxAcc * 3);
            //this.sColor = color(55,243,212);
        }
        else if(vel > this.maxVel) {
            this.internal_acc = this.velocity.copy();
            this.internal_acc.setMag(this.maxAcc * 3);
            this.internal_acc.mult(-1);
            //this.sColor = color(243,118,55);
        }

        // try to keep the boids inside the walls
            // if(this.position.x < this.wallThr) {this.internal_acc.x += this.maxAcc*3;}
            // if(this.position.x > width - this.wallThr) {this.internal_acc.x += -this.maxAcc*3;}
            // if(this.position.y < this.wallThr) {this.internal_acc.y += this.maxAcc*3;}
            // if(this.position.y > height - this.wallThr) {this.internal_acc.y += -this.maxAcc*3;}
        
        this.velocity.add(this.internal_acc);
        
        if(this.position.x < 0)     this.position.x += width;
        if(this.position.x > width) this.position.x -= width;
        if(this.position.y < 0)     this.position.y += height;
        if(this.position.y > height)this.position.y -= height; 
    }
}