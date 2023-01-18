#version 330 core
out vec4 FragColor;

float near = 0.01;
float far = 100.0;

float linearizarProfundidad(float depth)
{
	float ndc = depth * 2.0 - 1.0;
	return (2.0 * near * far) / (far + near - ndc * (far - near));
}

void main()
{             
    //FragColor = vec4(1.0, 1.0, 1.0, 1.0);	
	// Renderizado de profundidad
	//FragColor = vec4(vec3(gl.FragCoord.z), 1.0);
	FragColor = vec4(vec3(linearizarProfundidad(gl_FragCoord.z)), 1.0);
}
