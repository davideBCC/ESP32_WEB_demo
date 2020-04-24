class World {

    worldGrid = [];
    img = createImage(width, height);

    constructor() {
        this.img.loadPixels();
        for(let i=0;i<width;i++) {
            let row = [];
            for(let j=0; j<height; j++) {
                row.push(this.height(createVector(i,j)));
                let c = map(row[row.length - 1],0,1,0,50);
                this.img.set(i, j, color(c, c, c));
            }
            this.worldGrid.push(row);
        }
        this.img.updatePixels();
    }

    draw() {
        image(this.img, 0, 0);
    }

    height(location) {
        let temp = p5.Vector.div(location,100);
        return noise(temp.x,temp.y);
    }

    acceleration(location) {
        let heightTop = this.height(createVector(0,-1).add(location));
        let heightBot = this.height(createVector(0,1).add(location));
        let heightLh = this.height(createVector(-1,0).add(location));
        let heightRh = this.height(createVector(1,0).add(location));
        let acc = createVector(heightLh - heightRh, heightTop - heightBot );
        return p5.Vector.mult(acc,1);
    }
}