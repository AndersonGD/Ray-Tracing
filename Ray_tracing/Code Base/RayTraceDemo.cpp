#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "Scene.h"
#include "Objects.h"
#include "Rays.h"
#include "Camera.h"
#include "Vector3.h"
#include <GL\glut.h>


#define filterWidth 8
#define filterHeight 8

const int sc_w = 400;
const int sc_h = 400;

//Blur filter matrix
double blurFilter[filterHeight][filterWidth] =
{
	0, 0, 0, 1, 0, 0, 0,
	0, 0, 1, 1, 1, 0, 0,
	0, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 0,
	0, 0, 1, 1, 1, 0, 0,
	0, 0, 0, 1, 0, 0, 0,
};

double factor = 1.0 / 25.0;
double bias = 0.0;

void ApplyFilter(float *pixels, int w, int h);
float perlin(float x, float y);
float* GetEdges(float *pixels, int width, int height);
float **CalcularIntesidadesMedias(float **intensidades, int tamanhoCelula);
float * ApplyHDR(float *pixinfo, int width, int height);

using namespace std;

int width = 0;
int height = 0;
bool NPR = true;

Scene* world = new Scene();

//Rotation °ª
float Rot_num = 0.5f;

//GlutDisplayFunc(drawScene)
void draw(void)
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	float* pixinfo = new float[3*width*height];
	float* aux = new float[3 * width*height];

	//Rot_num 
	Rot_num += 0.05f;

	//Light Rotation 
	Vector3 RotLight(50 + cos(Rot_num)*50, 100, 5 + 5 * sin(Rot_num));
	world->lights.begin()->position = RotLight;

	//Gold 
	Vector3 RotBall(0.0f + 3.0f * cos(Rot_num), 2.0f, 12.0f + 3.0f * sin(Rot_num));
	list<Object*>::iterator it =  world->objects.begin();
	it++; it++; it++; 
	(*it)->position = RotBall;

	
	Vector3 cameraPoint(0.0f , 0.0f, -5.0f + sin(Rot_num) * -5.0f);
	Vector3 left_down_plane(-1.0f , -1.0f, 0.5f);
	Vector3 up_plane(0.0f, 2.0f, 0.0f);
	Vector3 right_plane(2.0f , 0.0f, 0.0f);

	int l = sizeof(float) * (int)pixinfo;
	
	//ViewPlane
	for(int h = 0; h < height; h++)
	{
		for(int w = 0; w < width; w++)
		{
			float wRatio = (float)w / width;
			float hRatio = (float)h / height;
			Vector3 hPoint = left_down_plane + (up_plane * hRatio);
			Vector3 wPoint = left_down_plane + (right_plane * wRatio);

			//Plane
			Vector3 plane_point(wPoint.v[0], hPoint.v[1], 0.5f + sin(Rot_num) * -5.0f);

			//Camera -> (Ray Tracing)
			Ray ray(cameraPoint, plane_point - cameraPoint);
			ray.shoot(world->objects, world->lights, 5, true, true);

			//rgb
			float med = (ray.color.v[0] + ray.color.v[1] + ray.color.v[2]) / 3;
			aux [h*width*3 + w*3 +0] = med; //r
			aux [h*width*3 + w*3 +1] = med; //g
			aux [h*width*3 + w*3 +2] = med; //b

			Ray ray2(cameraPoint, plane_point - cameraPoint);
			ray2.shoot(world->objects, world->lights, 5, true, false);

			//rgb
			
			pixinfo[h*width * 3 + w * 3 + 0] = ray.color.v[0]; //r
			pixinfo[h*width * 3 + w * 3 + 1] = ray.color.v[1]; //g
			pixinfo[h*width * 3 + w * 3 + 2] = ray.color.v[2]; //b
			
		}
		
		cout <<  h;
		cout << " : ";
		cout << height  << endl;
		
	}
	
	float * edges = GetEdges(aux, width, height);

	//filter
	//ApplyFilter(&*pixinfo, width, height);
	//float *hdr = ApplyHDR(pixinfo, width, height);

	

	glDrawPixels(width, height, GL_RGB, GL_FLOAT, edges);
	glFlush();

	delete [] pixinfo;

}

//scene objects creation
void setupComponentInsert(void)
{

	Material red;
	red.ambient.set(0.17f, 0.012f, 0.012f);
	red.diffuse.set(0.61f, 0.04f, 0.04f);
	red.specular.set(0.73f, 0.63f, 0.63f);
	red.emittance.set(0.0f, 0.0f, 0.0f);
	red.shininess = 60;
	red.reflection = 0.3f;
	red.transmission = 0.8f;
	red.refractiveIdx = 1.5f;

	Material green;
	green.ambient.set(0.0f, 0.0f, 0.0f);
	green.diffuse.set(0.1f, 0.35f, 0.1f);
	green.specular.set(0.45f, 0.55f, 0.45f);
	green.emittance.set(0.2f, 0.2f, 0.2f);
	green.shininess = 5;
	green.reflection = 0.5f;
	green.shininess = 30;

	Material glass;
	glass.ambient.set(0.0f, 0.0f, 0.0f);
	glass.diffuse.set(0.0f, 0.0f, 0.0f);
	glass.specular.set(0.9f, 0.9f, 0.9f);
	glass.shininess = 60;
	glass.reflection = 0.9f;
	glass.transmission = 0.9f;
	glass.refractiveIdx = 1.0f;

	Material gold;
	gold.ambient.set(0.247f, 0.2f, 0.075f);
	gold.diffuse.set(0.75f, 0.606f, 0.227f);
	gold.specular.set(0.628f, 0.556f, 0.366f);
	gold.shininess = 40;
	gold.reflection = 0.3f;
	gold.transmission = 0.0f;
	gold.refractiveIdx = 0.0f;
	
	Sphere* one = new Sphere(Vector3(-0.3f, 0.0f, 8.0f), 1.0f);
	one->setMaterial(glass);
	world->addObject(one);

	Sphere* two = new Sphere(Vector3(0.0f, 1.5f, 12.0f), 2.0f);
	two->setMaterial(red);
	world->addObject(two);

	Sphere* three = new Sphere(Vector3(1.5f, 0.0f, 9.0f), 0.6f);
	three->setMaterial(green);
	world->addObject(three);
	
	Sphere* four = new Sphere(Vector3(-1.0f, 2.0f, 9.0f), 0.6f);
	four->setMaterial(gold);
	world->addObject(four);

	Plane* plane1 = new Plane(Vector3(0.0f, -2.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), 100.0f, 100.0f);
	Material floor;
	floor.ambient.set(0.75f, 0.25f, 0.35f);
	floor.diffuse.set(0.52f, 0.5f, 0.5f);
	floor.specular.set(0.79f, 0.2f, 0.3f);
	floor.shininess = 8;
	floor.reflection = 0.5f;
	floor.transmission = 0.0f;
	floor.refractiveIdx = 0.0f;
	plane1->setMaterial(floor);
	world->addObject(plane1);

	//Plane* plane2 = new Plane(Vector3(0.0f, -5.0f, -10.0f), Vector3(0.0f, 1.0f, 0.0f), 100.0f, 100.0f);
	///*Material floor;
	//floor.ambient.set(0.75f, 0.25f, 0.35f);
	//floor.diffuse.set(0.52f, 0.5f, 0.5f);
	//floor.specular.set(0.79f, 0.2f, 0.3f);
	//floor.shininess = 8;
	//floor.reflection = 0.5f;
	//floor.transmission = 0.0f;
	//floor.refractiveIdx = 0.0f;*/
	//plane1->setMaterial(green);
	//world->addObject(plane2);


	Light light;
	light.color.set(1.0f, 1.0f, 1.0f);
	light.position.set(100.0f, 100.0f, 5.0f);
	world->addLight(light);

}

void init(void) 
{
	glClearColor(1.0, 1.0, 1.0, 0.0); 
	setupComponentInsert();
}

// Resize
// re-draw
void resize(int w, int h)
{
	width = w;
	height = h;

	
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	
	glOrtho(0.0, 100.0, 0.0, 100.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glutPostRedisplay();
}

int main(int argc, char **argv) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB); 
	glutInitWindowSize(400, 400);
	glutInitWindowPosition(100, 100); 
	glutCreateWindow("TRABALHO GA");

	init(); 
     
	glutReshapeFunc(resize);  
	glutDisplayFunc(draw);

	glutMainLoop(); 

	return 0;  
}

float* ApplyHDR(float *pixels, int width, int height) {

	float **intensidades = new float*[width];
	for (int i = 0; i < width; i++)
	{
		intensidades[i] = new float[height];
	}

	float **media;



	for (int h = 0; h < height; h++)
	{
		for (int w = 0; w < width; w++)
		{
			int red = h*width * 3 + w * 3 + 1;
			int green = h*width * 3 + w * 3 + 2;
			int blue = h*width * 3 + w * 3 + 3;
			
			intensidades[h][w] = pixels[red] * 0.27 + pixels[green] * 0.67 + pixels[blue] * 0.16;
		}
	}

	//Computar a matriz de intensidades médias por região
	media = CalcularIntesidadesMedias(intensidades, 1);

	//Aplicar intensidades médias sobre imagem do rendering

	float* pixinfo = new float[3 * width*height];

	int l = sizeof(float) * (int)pixinfo;

	for (int h = 0; h < height - 2; h++)
	{
		for (int w = 0; w < width - 2; w++)
		{
			int index = h*width * 3 + w * 3 + 0;
			float v = (0.15f / media[h][w]);
			pixinfo[index] = ( v * pixels[index]) + pixels[index];

			int index2 = h*width * 3 + w * 3 + 1;
			float v2 = (0.15f / media[h][w]);
			pixinfo[index2] = (v2 * pixels[index2]) + pixels[index];

			int index3 = h*width * 3 + w * 3 + 2;
			float v3 = (0.15f / media[h][w]);
			pixinfo[index3] = (v3 * pixels[index3]) + pixels[index];
		}
	}
	return pixinfo;
}

float ** CalcularIntesidadesMedias(float **intensidades, int tamanhoCelula) {
	float **media = new float*[width];
	for (int i = 0; i < width; i++)
	{
		media[i] = new float[height];
	}



	for (int h = 1; h < height-2; h+=2)
	{
		for (int w = 1; w < width-2; w+=2)
		{
			float media2 = 0.0f;
			media2 += intensidades[h - 1][w - 1];
			media2 += intensidades[h - 0][w - 1];
			media2 += intensidades[h + 1][w - 1];

			media2 += intensidades[h - 1][w];
			media2 += intensidades[h][w];
			media2 += intensidades[h + 1][w];

			media2 += intensidades[h - 1][w + 1];
			media2 += intensidades[h][w + 1];
			media2 += intensidades[h + 1][w + 1];

			media2 = media2 / 9;

			
			media[h - 1][w - 1] = media2;
			media[h - 0][w - 1] = media2;
			media[h + 1][w - 1] = media2;

			media[h - 1][w] = media2;
			media[h][w] = media2;
			media[h + 1][w] = media2;

			media[h - 1][w + 1] = media2;
			media[h][w + 1] = media2;
			media[h + 1][w + 1] = media2;
		}
	}


	return media;
}

void ApplyFilter(float *pixels, int width, int height) {
	//apply the filter
	for (int h = 0; h < height; h++)
	{
		for (int w = 0; w < width; w++)
		{
			double red = 0.0, green = 0.0, blue = 0.0;

			//multiply every value of the filter with corresponding image pixel
			for (int filterY = 0; filterY < filterHeight; filterY++)
			{
				for (int filterX = 0; filterX < filterWidth; filterX++)
				{
					int index = h*width * 3 + w * 3 + 0;
					red += pixels[index] * blurFilter[filterY][filterX];
					

					index = h*width * 3 + w * 3 + 1;
					green += pixels[index] * blurFilter[filterY][filterX];

					index = h*width * 3 + w * 3 + 2;
					blue += pixels[index] * blurFilter[filterY][filterX];

				}
			}
			//truncate values smaller than zero and larger than 255
			float finalColor = (float(factor * red + bias));
			pixels[h*width * 3 + w * 3 + 0] = finalColor;
			
			finalColor = (float(factor * green + bias));
			pixels[h*width * 3 + w * 3 + 1] = finalColor;

			finalColor = (float(factor * blue + bias));
			pixels[h*width * 3 + w * 3 + 2] = finalColor;
		}
	}
}

float* GetEdges(float *pixels, int width, int height) {

	int Gx[3][3] =
	{
		-1, 0, 1,
		-2, 0, 2,
		-1, 0, 1
	};

	int Gy[3][3] =
	{
		-1,-2,-1,
		0, 0, 0,
		1, 2, 1
	};
	
	short x_weight = 0;
	short y_weight = 0;
	float*edges = new float[3 * width*height];
	for (int h = 0; h < height-2; h++)
	{
		for (int w = 0; w < width-2; w++)
		{
			int index = h*width * 3 + w * 3 + 0;
			float pix = pixels[index];
			int gx = Gx[h][w];
			int gy = Gy[h][w];
			x_weight += (pix * 255) * gx;
			y_weight += (pix * 255) * gy;

			float val = (abs(x_weight) + abs(y_weight)) / 100;

			if (val>=255)
				val = 255;
			else if (val<150)
				val = 0;
			edges[index] = (255 - (float)(val));
			edges[index+1] = (255 - (float)(val));
			edges[index+2] = (255 - (float)(val));
		}
	}
	
	


	return edges;

}