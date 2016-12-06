#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <playshader.h>
#include <glhelper.h>
#include <nemomisc.h>

struct playshader *nemoplay_shader_create(void)
{
	struct playshader *shader;

	shader = (struct playshader *)malloc(sizeof(struct playshader));
	if (shader == NULL)
		return NULL;
	memset(shader, 0, sizeof(struct playshader));

	return shader;
}

void nemoplay_shader_destroy(struct playshader *shader)
{
	if (shader->fbo > 0)
		glDeleteFramebuffers(1, &shader->fbo);
	if (shader->dbo > 0)
		glDeleteRenderbuffers(1, &shader->dbo);

	if (shader->texy > 0)
		glDeleteTextures(1, &shader->texy);
	if (shader->texu > 0)
		glDeleteTextures(1, &shader->texu);
	if (shader->texv > 0)
		glDeleteTextures(1, &shader->texv);

#ifdef NEMOUX_WITH_OPENGL_PBO
	if (shader->pboy > 0)
		glDeleteBuffers(1, &shader->pboy);
	if (shader->pbou > 0)
		glDeleteBuffers(1, &shader->pbou);
	if (shader->pbov > 0)
		glDeleteBuffers(1, &shader->pbov);
#endif

	if (shader->shaders[0] > 0)
		glDeleteShader(shader->shaders[0]);
	if (shader->shaders[1] > 0)
		glDeleteShader(shader->shaders[1]);
	if (shader->program > 0)
		glDeleteProgram(shader->program);

	free(shader);
}

int nemoplay_shader_set_texture(struct playshader *shader, int32_t width, int32_t height)
{
	if (shader->texy > 0)
		glDeleteTextures(1, &shader->texy);
	if (shader->texu > 0)
		glDeleteTextures(1, &shader->texu);
	if (shader->texv > 0)
		glDeleteTextures(1, &shader->texv);

#ifdef NEMOUX_WITH_OPENGL_PBO
	if (shader->pboy > 0)
		glDeleteBuffers(1, &shader->pboy);
	if (shader->pbou > 0)
		glDeleteBuffers(1, &shader->pbou);
	if (shader->pbov > 0)
		glDeleteBuffers(1, &shader->pbov);
#endif

	glGenTextures(1, &shader->texy);
	glBindTexture(GL_TEXTURE_2D, shader->texy);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			width,
			height,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &shader->texu);
	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width / 2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			width / 2,
			height / 2,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &shader->texv);
	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width / 2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			width / 2,
			height / 2,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

#ifdef NEMOUX_WITH_OPENGL_PBO
	glGenBuffers(1, &shader->pboy);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pboy);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glGenBuffers(1, &shader->pbou);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pbou);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height / 4, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glGenBuffers(1, &shader->pbov);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pbov);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height / 4, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
#endif

	shader->texture_width = width;
	shader->texture_height = height;
	shader->texture_linesize = width;

	return 0;
}

int nemoplay_shader_set_texture_linesize(struct playshader *shader, int32_t linesize)
{
	shader->texture_linesize = linesize;

	return 0;
}

int nemoplay_shader_set_viewport(struct playshader *shader, uint32_t texture, int32_t width, int32_t height)
{
	if (shader->fbo > 0)
		glDeleteFramebuffers(1, &shader->fbo);
	if (shader->dbo > 0)
		glDeleteRenderbuffers(1, &shader->dbo);

	gl_create_fbo(texture, width, height, &shader->fbo, &shader->dbo);

	shader->texture = texture;
	shader->viewport_width = width;
	shader->viewport_height = height;

	return 0;
}

int nemoplay_shader_prepare(struct playshader *shader, const char *vertex_source, const char *fragment_source)
{
	shader->program = gl_compile_program(vertex_source, fragment_source, &shader->shaders[0], &shader->shaders[1]);
	if (shader->program == 0)
		return -1;
	glUseProgram(shader->program);
	glBindAttribLocation(shader->program, 0, "position");
	glBindAttribLocation(shader->program, 1, "texcoord");

	shader->utexy = glGetUniformLocation(shader->program, "texy");
	shader->utexu = glGetUniformLocation(shader->program, "texu");
	shader->utexv = glGetUniformLocation(shader->program, "texv");

	return 0;
}

void nemoplay_shader_finish(struct playshader *shader)
{
	glDeleteShader(shader->shaders[0]);
	glDeleteShader(shader->shaders[1]);
	glDeleteProgram(shader->program);

	shader->shaders[0] = 0;
	shader->shaders[1] = 0;
	shader->program = 0;
}

int nemoplay_shader_update(struct playshader *shader, uint8_t *y, uint8_t *u, uint8_t *v)
{
#ifdef NEMOUX_WITH_OPENGL_PBO
	GLubyte *ptr;

	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pboy);

	ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, shader->texture_width * shader->texture_height, GL_MAP_WRITE_BIT);
	memcpy(ptr, y, shader->texture_width * shader->texture_height);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	glBindTexture(GL_TEXTURE_2D, shader->texy);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_linesize);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			shader->texture_width,
			shader->texture_height,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pbou);

	ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, shader->texture_width * shader->texture_height / 4, GL_MAP_WRITE_BIT);
	memcpy(ptr, u, shader->texture_width * shader->texture_height / 4);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_linesize / 2);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			shader->texture_width / 2,
			shader->texture_height / 2,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pbov);

	ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, shader->texture_width * shader->texture_height / 4, GL_MAP_WRITE_BIT);
	memcpy(ptr, v, shader->texture_width * shader->texture_height / 4);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_linesize / 2);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			shader->texture_width / 2,
			shader->texture_height / 2,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
#else
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_linesize / 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width / 2,
			shader->texture_height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			v);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_linesize / 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width / 2,
			shader->texture_height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			u);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, shader->texy);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_linesize);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width,
			shader->texture_height,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			y);
	glBindTexture(GL_TEXTURE_2D, 0);
#endif

	return 0;
}

int nemoplay_shader_dispatch(struct playshader *shader)
{
	GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	glBindFramebuffer(GL_FRAMEBUFFER, shader->fbo);

	glViewport(0, 0, shader->viewport_width, shader->viewport_height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shader->program);
	glUniform1i(shader->utexy, 0);
	glUniform1i(shader->utexu, 1);
	glUniform1i(shader->utexv, 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shader->texy);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 0;
}
