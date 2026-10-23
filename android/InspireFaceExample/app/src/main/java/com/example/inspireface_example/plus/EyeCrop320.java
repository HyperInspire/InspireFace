package com.example.inspireface_example.plus;

/** eye_crop_320_v1, adapted from the service-referenced flash_lab/ModelMath.java.
 * Keep integer rounding and bilinear sampling identical to the protocol golden fixtures. */
public final class EyeCrop320 {
    public static final int SIDE=320, PIXELS=SIDE*SIDE;
    private EyeCrop320() {}
    public static int[] crop(int[] source,int width,int height,float ax0,float ay0,float bx0,float by0) {
        int ax=(int)ax0,ay=(int)ay0,bx=(int)bx0,by=(int)by0;
        if (source.length!=width*height || width<2 || height<2) throw new IllegalArgumentException("Invalid frame");
        int cx=(ax+bx)/2,cy=(ay+by)/2;
        int dx=ax-bx,dy=ay-by;
        int distance=(int)(float)Math.sqrt((float)(dx*dx)+(float)(dy*dy));
        int side=(int)((float)((int)(distance*.5*1.5)*2+2)*3.5f);
        if(side<=7 || distance<4)throw new IllegalArgumentException("Eye distance is too small");
        if(bx<=ax){int tx=ax,ty=ay;ax=bx;ay=by;bx=tx;by=ty;}
        int angle=(int)(-(float)Math.atan((float)(by-ay)/(float)(bx-ax))*180.0f/Math.PI);
        double scale=(float)SIDE/(float)side;
        return affine(source,width,height,scale,scale,angle,cx,cy,160,128);
    }
    public static int[] affine(int[] source,int width,int height,double sx,double sy,int angle,int cx,int cy,int tx,int ty) {
        double radians=angle*Math.PI/180.,c=Math.cos(radians),s=Math.sin(radians);
        double ox=cx-(c*tx/sx+s*ty/sy),oy=cy-(-s*tx/sx+c*ty/sy);
        int[] out=new int[PIXELS];double stepX=c/sx,stepY=s/sx;
        for(int row=0;row<SIDE;row++){
            double x=ox+s*row/sy,y=oy+c*row/sy;
            for(int col=0;col<SIDE;col++,x+=stepX,y-=stepY){
                int ix=(int)Math.floor(x),iy=(int)Math.floor(y),rgb=0xff000000;
                if(ix>=0&&iy>=0&&ix<width-1&&iy<height-1){
                    double fx=x-ix,fy=y-iy;
                    int p=iy*width+ix,a=source[p],b=source[p+1],d=source[p+width],e=source[p+width+1];
                    for(int shift=0;shift<=16;shift+=8){
                        double top=(1-fx)*((a>>>shift)&255)+fx*((b>>>shift)&255);
                        double bottom=(1-fx)*((d>>>shift)&255)+fx*((e>>>shift)&255);
                        int v=Math.max(0,Math.min(255,(int)(top+fy*(bottom-top))));rgb|=v<<shift;
                    }
                }
                out[row*SIDE+col]=rgb;
            }
        }
        return out;
    }
}
