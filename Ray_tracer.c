#include <SDL2/SDL.h>
#include <stdio.h>
#include<math.h>
#define WIDTH 900
#define HEIGHT 600
#define COLOR_WHITE 0xffffffff
#define COLOR_BLACK  0x00000000
#define COLOR_GREY 0xfefefefe
#define COLOR_RAY 0xF2DD4E
#define RAY_NUMBER 200

struct Circle{
    double x;
    double y;
    double r;
};
struct Ray{
    double x_s, y_s;
    double angle;
    double x_end,y_end; 
};
void FillCircle(SDL_Surface* surface, struct Circle circle,Uint32 color ){
    double radius_squared = pow(circle.r,2);
    for (double x = circle.x-circle.r; x<= circle.x + circle.r;x++){
        for(double y = circle.y - circle.r; y <= circle.y + circle.r;y++){ //This makes us go down grid from top left to down itteration
            double distance_squared = pow(x-circle.x,2)+ pow(y-circle.y,2);
            if(distance_squared < radius_squared){ //This just checks wheter the distance of the point is greater than radius or not 
                SDL_Rect pixel = (SDL_Rect){x,y,1,1}; 
                SDL_FillRect(surface , &pixel , color); //If that distance's is smaller then fill it 
            }
        }
    }
}

void generate_rays(struct Circle circle,struct Ray rays[RAY_NUMBER]){
    for(int i =0; i < RAY_NUMBER ;i++){
        double angle = ((double)i/RAY_NUMBER) * 2* M_PI;
        struct Ray ray = {circle.x,circle.y,angle};
        rays[i] = ray;
    }
}

void FillRays(SDL_Surface* surface, struct Ray rays[RAY_NUMBER],Uint32 color,struct Circle object ){
    for(int i = 0; i < RAY_NUMBER; i++){
        struct Ray ray = rays[i];

        int end_of_screen = 0;
        int object_hit = 0;

        double step = 1;
        double x_draw = ray.x_s;
        double y_draw = ray.y_s;
        double radius_squared = pow(object.r,2);
    
        while( !end_of_screen && !object_hit){
            x_draw += step*cos(ray.angle);// Sin = Horizontal Movement & Cos = Vertical Movement 
            y_draw += step*sin(ray.angle); // These steps are about movinng forward one step 

            SDL_Rect  pixel = (SDL_Rect){x_draw,y_draw,1,2};  
            SDL_FillRect(surface,&pixel,color); //Once you move that step then color on that step
            if(x_draw <0|| x_draw > WIDTH) //Both conditions are to check if the ray is still in window or not 
                end_of_screen = 1;
            if(y_draw <0||y_draw > HEIGHT)
                end_of_screen =1;
            //The code below breaks the rays when they come in the radius of the shadow circle ;) cheap work
            double distance_squared = pow(x_draw-object.x,2) + pow(y_draw-object.y,2);
            if(distance_squared < radius_squared){
                break;
            }
        }
    }
}
int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Raytracaing",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WIDTH, HEIGHT,0);
    
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    
    struct Circle circle = {200,200,80};
    struct Circle shadow_circle = {550,325,100};
    struct SDL_Rect erase_rect = {0,0,WIDTH,HEIGHT};

    struct Ray rays[RAY_NUMBER];
    generate_rays(circle,rays);

    int simmulation_running = 1;
    SDL_Event event;

    while( simmulation_running)
    {
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                simmulation_running = 0;
            }
            if (event.type == SDL_MOUSEMOTION && event.motion.state != 0 )
            {
                circle.x = event.motion.x;
                circle.y = event.motion.y;
                generate_rays(circle,rays);
            }
            SDL_FillRect(surface, &erase_rect,COLOR_BLACK);
            FillCircle(surface,circle, COLOR_RAY);

            FillCircle(surface,shadow_circle,COLOR_GREY);
            FillRays(surface, rays,COLOR_RAY,shadow_circle);
    
            SDL_UpdateWindowSurface(window);
            SDL_Delay(10);
        }
    }
}
