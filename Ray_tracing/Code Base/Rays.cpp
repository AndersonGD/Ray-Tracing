#include <math.h>
#include <list>
#include <GL/glut.h>
#include "Vector3.h"
#include "Rays.h"
#include "Objects.h"
#include "Collisions.h"

using namespace std;

Ray::Ray(const Vector3& o, const Vector3& d)
{
	origin = o;
	direction = d.normal();
	depth = 0;
	distance = 0.0f;
	color = NULL;
	mediumRefraction = 1.0f;
	insideSphere = false;
}

Ray::Ray(const Vector3& o, const Vector3& d, int n)
{
	origin = o;
	direction = d.normal();
	depth = n;
	distance = 0.0f;
	color = NULL;
	mediumRefraction = 1.0f;
	insideSphere = false;
}

Ray::~Ray()
{
}

bool compare_distance(const Collision& col1, const Collision& col2)
{
	return col1.distance < col2.distance;
}

Collision Ray::getFirstCollision(const list<Object*>& scene)
{
	Collision firstCol;
	list<Collision> collisions;

	
	list<Object*>::const_iterator itr = scene.begin();
	for( ; itr != scene.end(); itr++)
	{
		// Create Collision
		Collision col = (*itr)->collideWithRay((*this));

		if(col.distance > -1.0f)
			collisions.push_back(col);
	}

	
	if (collisions.size() == 0)
	{
		firstCol.distance = -1.0f;
		return firstCol;
	}

	
	collisions.sort(compare_distance);

	return *(collisions.begin());
}

Vector3 Ray::shoot(const list<Object*>& scene, const list<Light>& lights, int depth, bool npr, bool aux)
{
	Collision col = getFirstCollision(scene);

	// get FirstCollison
	if(col.distance > 0.0f)
	{
		distance = col.distance;
		// emittance 
		color += col.material.emittance;
		list<Light>::const_iterator itr = lights.begin();
		for( ; itr != lights.end(); itr++)
		{
			
			Vector3 toLight = ((*itr).position - col.point).normal();
			Ray toLightRay(col.point, toLight);
			bool far_ = toLightRay.is_far(scene, (*itr));
				
			
			color += (col.material.ambient).scale((*itr).color);
			
			if(far_) 
			{
				continue;
			}
			else
			{
				
				float normAngle = toLight * col.normal;

				
				float specAngle = ((toLight + (direction * -1.0f)) * 0.5f).normal() * col.normal;

				if (aux == false) {
					// Diffuse -> JA É O SUFICIENTE DIFUSA? 
					if (npr == false) {
						color += (col.material.diffuse).scale((*itr).color) * normAngle;

					}
					else
					{
						if (normAngle >= 0.5f) {
							normAngle = 1.0f;
						}
						else if (normAngle < 0.5f) {
							normAngle = 0.5f;
						}
						
						color += (col.material.diffuse).scale((*itr).color) * normAngle;
					}
						// Specular -> MESMA PERGUNTA
						if (specAngle > 0.0f)
							color += col.material.specular.scale((*itr).color) * pow(specAngle, col.material.shininess);
					

				}
			}
		}
	
		if(aux == false){
			//Refraction
			if (depth > 0)
			{

				if (col.material.reflection > 0.0f && !insideSphere)
				{
					Vector3 dir = direction.reflect(col.normal).normal();
					Ray reflect(col.point, dir);
					reflect.insideSphere = insideSphere;
					Vector3 reflectColor = reflect.shoot(scene, lights, depth - 1, npr, aux);
					color += reflectColor  * col.material.reflection;
				}
				else {}


				if (col.material.transmission > 0.0f)
				{

					float c1 = (direction * col.normal) * -1.0f;
					float n = mediumRefraction / (mediumRefraction == 1.0f ? col.material.refractiveIdx : 1.0f);
					float c2 = sqrt(1 - pow(n, 2) * (1 - pow(c1, 2)));

					Vector3 norm = (insideSphere ? col.normal * -1.0f : col.normal);
					Vector3 dir = (direction * n) + (norm * (n * c1 - c2));
					Ray refract(col.point, dir);


					refract.insideSphere = !insideSphere;
					refract.mediumRefraction = col.material.refractiveIdx;

					Vector3 refractColor = refract.shoot(scene, lights, depth - 1, npr, aux);
					color += refractColor * col.material.transmission;

					color *= 0.5f;
				}
				else {}
			}
		}

		return color;
	}
	
	color.set(0.0f,0.0f,0.0f);
	return color;
}


bool Ray::is_far(const list<Object*>& scene, const Light& light)
{
	Collision col = getFirstCollision(scene);
	bool hit = false;
	
	// No distance
	if(col.distance <= -1.0f)
	{
		return hit;
	}

	// Distance
	Vector3 diff = light.position - origin;
	float distToLight = diff.size();

	
	if(distToLight > col.distance)
	{
		hit = true;
	}
	
	return hit;
}