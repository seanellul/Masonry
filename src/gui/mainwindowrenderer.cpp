/*	
	This file is part of Ingnomia https://github.com/rschurade/Ingnomia
    Copyright (C) 2017-2020  Ralph Schurade, Ingnomia Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "mainwindowrenderer.h"

#include <cstring>

#include "../game/game.h" //TODO only temporary

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../base/vptr.h"
#include "../game/gamemanager.h"
#include "../game/plant.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "eventconnector.h"
#include "mainwindow.h"
#include "aggregatorselection.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QMessageBox>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QTimer>

namespace
{
class DebugScope
{
public:
	DebugScope() = delete;
	DebugScope( const char* c )
	{
#ifndef __APPLE__
		static GLuint counter    = 0;
		QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
		f->glPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION, counter++, static_cast<GLsizei>( strlen( c ) ), c );
#else
		Q_UNUSED( c );
#endif
	}
	~DebugScope()
	{
#ifndef __APPLE__
		QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
		f->glPopDebugGroup();
#endif
	}
};
} // namespace

MainWindowRenderer::MainWindowRenderer( MainWindow* parent ) :
	QObject( parent ),
	m_parent( parent )
{
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalWorldParametersChanged, this, &MainWindowRenderer::cleanupWorld );

	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalTileUpdates, this, &MainWindowRenderer::onTileUpdates );
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalAxleData, this, &MainWindowRenderer::onAxelData );
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalThoughtBubbles, this, &MainWindowRenderer::onThoughtBubbles );
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalCenterCamera, this, &MainWindowRenderer::onCenterCameraPosition );
	connect( Global::eventConnector, &EventConnector::signalInMenu, this, &MainWindowRenderer::onSetInMenu );

	connect( Global::eventConnector->aggregatorSelection(), &AggregatorSelection::signalUpdateSelection, this, &MainWindowRenderer::onUpdateSelection, Qt::QueuedConnection );

	// Full polling of initial state on load
	connect( this, &MainWindowRenderer::fullDataRequired, Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::onAllTileInfo );
	connect( this, &MainWindowRenderer::fullDataRequired, Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::onThoughtBubbleUpdate );
	connect( this, &MainWindowRenderer::fullDataRequired, Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::onAxleDataUpdate );

	connect( this, &MainWindowRenderer::signalCameraPosition, Global::eventConnector, &EventConnector::onCameraPosition );

	qDebug() << "initialize GL ...";
	connect( m_parent->context(), &QOpenGLContext::aboutToBeDestroyed, this, &MainWindowRenderer::cleanup );
	connect( this, &MainWindowRenderer::redrawRequired, m_parent, &MainWindow::redraw );

	if ( !initializeOpenGLFunctions() )
	{
		qDebug() << "failed to initialize OpenGL - make sure your graphics card and driver support OpenGL 4.1";
		qCritical() << "failed to initialize OpenGL functions core 4.1 - exiting";
		QMessageBox msgBox;
		msgBox.setText( "Failed to initialize OpenGL - make sure your graphics card and driver support OpenGL 4.1" );
		msgBox.exec();
		exit( 0 );
	}

	qDebug() << "[OpenGL]" << reinterpret_cast<char const*>( glGetString( GL_VENDOR ) );
	qDebug() << "[OpenGL]" << reinterpret_cast<char const*>( glGetString( GL_VERSION ) );
	qDebug() << "[OpenGL]" << reinterpret_cast<char const*>( glGetString( GL_RENDERER ) );

	qDebug() << m_parent->context()->format();

#ifndef __APPLE__
	QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
	GLDEBUGPROC logHandler   = []( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam ) -> void
	{
		static const std::unordered_map<GLenum, const char*> debugTypes = {
			{ GL_DEBUG_TYPE_ERROR, "Error" },
			{ GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "DeprecatedBehavior" },
			{ GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "UndefinedBehavior" },
			{ GL_DEBUG_TYPE_PORTABILITY, "Portability" },
			{ GL_DEBUG_TYPE_PERFORMANCE, "Performance" },
			{ GL_DEBUG_TYPE_MARKER, "Marker" },
			{ GL_DEBUG_TYPE_OTHER, "Other" },
			{ GL_DEBUG_TYPE_PUSH_GROUP, "Push" },
			{ GL_DEBUG_TYPE_POP_GROUP, "Pop" }
		};
		static const std::unordered_map<GLenum, const char*> severities = {
			{ GL_DEBUG_SEVERITY_LOW, "low" },
			{ GL_DEBUG_SEVERITY_MEDIUM, "medium" },
			{ GL_DEBUG_SEVERITY_HIGH, "high" },
			{ GL_DEBUG_SEVERITY_NOTIFICATION, "notify" },
		};
		if ( severity == GL_DEBUG_SEVERITY_NOTIFICATION && !Global::debugOpenGL )
			return;
		// Only want to handle these from dedicated graphic debugger
		if ( type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP )
			return;
		qDebug() << "[OpenGL]" << debugTypes.at( type ) << " " << severities.at(severity) << ":" << message;
	};
	glEnable( GL_DEBUG_OUTPUT );
	f->glDebugMessageCallback( logHandler, nullptr );
#endif
}
MainWindowRenderer ::~MainWindowRenderer()
{
}

void MainWindowRenderer::initializeGL()
{
	float vertices[] = {
		// Wall layer
		0.f, .8f, 1.f, // top left
		0.f, .2f, 1.f, // bottom left
		1.f, .8f, 1.f, // top right
		1.f, .2f, 1.f, // bottom right

		// floor layer
		0.f, .5f, 0.f, // top left
		0.f, .2f, 0.f, // bottom left
		1.f, .5f, 0.f, // top right
		1.f, .2f, 0.f, // bottom right
	};

	constexpr GLushort indices[] = {
		0, 1, 3, // Wall 1
		2, 0, 3, // Wall 2
		4, 5, 7, // Floor 1
		6, 4, 7, // Floor 2
		// Wall again for BTF rendering
		0, 1, 3, // Wall 1
		2, 0, 3, // Wall 2
	};

	if ( !initShaders() )
	{
		//qCritical() << "failed to init shaders - exiting";
	}

	m_vao.create();
	QOpenGLVertexArrayObject::Binder vaoBinder( &m_vao );

	glGenBuffers( 1, &m_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), (void*)0 );
	glEnableVertexAttribArray( 0 );

	glGenBuffers( 1, &m_vibo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_vibo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

	updateRenderParams();

	qDebug() << "initialize GL - done";
}

void MainWindowRenderer::reloadShaders()
{
	m_reloadShaders = true;
}

void MainWindowRenderer::cleanup()
{
	m_parent->makeCurrent();
	m_worldShader.reset();
	m_thoughtBubbleShader.reset();
	m_selectionShader.reset();
	m_axleShader.reset();
	m_postProcessShader.reset();
	m_brightExtractShader.reset();
	m_blurShader.reset();

	cleanupPostProcess();

	glDeleteBuffers( 1, &m_vbo );
	glDeleteBuffers( 1, &m_vibo );

	m_parent->doneCurrent();

	m_reloadShaders = true;
}

void MainWindowRenderer::cleanupWorld()
{
	m_parent->makeCurrent();
	glDeleteTextures( 16, m_textures );
	memset( m_textures, 0, sizeof( m_textures ) );
	glDeleteBuffers( 1, &m_tileBo );
	m_tileBo = 0;
	glDeleteTextures( 1, &m_tileTbo );
	m_tileTbo = 0;
	m_texesInitialized = false;

	m_parent->doneCurrent();

	m_pendingUpdates.clear();
	m_selectionData.clear();
	m_thoughBubbles = ThoughtBubbleInfo();
	m_axleData      = AxleDataInfo();
}

void MainWindowRenderer::initPostProcess()
{
	// Fullscreen VAO — no buffers needed, vertex shader generates coords from gl_VertexID
	glGenVertexArrays( 1, &m_fullscreenVao );

	// Scene FBO — render world to this instead of default framebuffer
	glGenFramebuffers( 1, &m_sceneFbo );
	glGenTextures( 1, &m_sceneColorTex );
	glGenTextures( 1, &m_sceneDepthTex );

	// Bright extraction FBO (half resolution)
	glGenFramebuffers( 1, &m_brightFbo );
	glGenTextures( 1, &m_brightColorTex );

	// Ping-pong blur FBOs (half resolution)
	glGenFramebuffers( 2, m_pingPongFbo );
	glGenTextures( 2, m_pingPongTex );

	// Allocate textures at current size
	if ( m_width > 0 && m_height > 0 )
	{
		resizePostProcess( m_width, m_height );
	}

	m_postProcessReady = true;
}

void MainWindowRenderer::resizePostProcess( int w, int h )
{
	if ( !m_sceneFbo )
		return;

	// Use physical pixel dimensions for Retina displays
	int pw = w * m_dpr;
	int ph = h * m_dpr;

	// Scene color (full res, RGBA16F for HDR headroom)
	glBindTexture( GL_TEXTURE_2D, m_sceneColorTex );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA16F, pw, ph, 0, GL_RGBA, GL_FLOAT, nullptr );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	// Scene depth (full res)
	glBindTexture( GL_TEXTURE_2D, m_sceneDepthTex );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, pw, ph, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	// Attach to scene FBO
	glBindFramebuffer( GL_FRAMEBUFFER, m_sceneFbo );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_sceneColorTex, 0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_sceneDepthTex, 0 );

	GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
	if ( status != GL_FRAMEBUFFER_COMPLETE )
	{
		qWarning() << "Scene FBO incomplete:" << status;
	}

	// Half-res for bloom (based on physical pixels)
	int hw = pw / 2;
	int hh = ph / 2;
	if ( hw < 1 ) hw = 1;
	if ( hh < 1 ) hh = 1;

	// Bright extract texture
	glBindTexture( GL_TEXTURE_2D, m_brightColorTex );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA16F, hw, hh, 0, GL_RGBA, GL_FLOAT, nullptr );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glBindFramebuffer( GL_FRAMEBUFFER, m_brightFbo );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_brightColorTex, 0 );

	// Ping-pong blur textures (half res)
	for ( int i = 0; i < 2; ++i )
	{
		glBindTexture( GL_TEXTURE_2D, m_pingPongTex[i] );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA16F, hw, hh, 0, GL_RGBA, GL_FLOAT, nullptr );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		glBindFramebuffer( GL_FRAMEBUFFER, m_pingPongFbo[i] );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingPongTex[i], 0 );
	}

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
}

void MainWindowRenderer::cleanupPostProcess()
{
	if ( m_sceneFbo )
	{
		glDeleteFramebuffers( 1, &m_sceneFbo );
		m_sceneFbo = 0;
	}
	if ( m_sceneColorTex )
	{
		glDeleteTextures( 1, &m_sceneColorTex );
		m_sceneColorTex = 0;
	}
	if ( m_sceneDepthTex )
	{
		glDeleteTextures( 1, &m_sceneDepthTex );
		m_sceneDepthTex = 0;
	}
	if ( m_brightFbo )
	{
		glDeleteFramebuffers( 1, &m_brightFbo );
		m_brightFbo = 0;
	}
	if ( m_brightColorTex )
	{
		glDeleteTextures( 1, &m_brightColorTex );
		m_brightColorTex = 0;
	}
	glDeleteFramebuffers( 2, m_pingPongFbo );
	m_pingPongFbo[0] = m_pingPongFbo[1] = 0;
	glDeleteTextures( 2, m_pingPongTex );
	m_pingPongTex[0] = m_pingPongTex[1] = 0;

	if ( m_fullscreenVao )
	{
		glDeleteVertexArrays( 1, &m_fullscreenVao );
		m_fullscreenVao = 0;
	}

	m_postProcessReady = false;
}

void MainWindowRenderer::renderPostProcess()
{
	// Use texture units 17-18 to avoid conflicting with tile textures (0-15) and TBO (16)
	// macOS OpenGL driver has issues with GL_TEXTURE_2D and GL_TEXTURE_2D_ARRAY on same unit
	const int PP_UNIT0 = 17;
	const int PP_UNIT1 = 18;

	int pw = m_width * m_dpr;
	int ph = m_height * m_dpr;

	glBindVertexArray( m_fullscreenVao );

	// Step 1: Bright extraction — extract bright pixels at half resolution
	{
		glBindFramebuffer( GL_FRAMEBUFFER, m_brightFbo );
		glViewport( 0, 0, pw / 2, ph / 2 );
		glClear( GL_COLOR_BUFFER_BIT );

		m_brightExtractShader->bind();
		glActiveTexture( GL_TEXTURE0 + PP_UNIT0 );
		glBindTexture( GL_TEXTURE_2D, m_sceneColorTex );
		m_brightExtractShader->setUniformValue( "uSceneColor", PP_UNIT0 );
		m_brightExtractShader->setUniformValue( "uThreshold", m_bloomThreshold );

		glDrawArrays( GL_TRIANGLES, 0, 3 );
		m_brightExtractShader->release();
	}

	// Step 2: Gaussian blur ping-pong at half resolution
	{
		m_blurShader->bind();
		bool horizontal = true;
		bool firstPass  = true;

		for ( int i = 0; i < m_bloomBlurPasses * 2; ++i )
		{
			glBindFramebuffer( GL_FRAMEBUFFER, m_pingPongFbo[horizontal ? 1 : 0] );
			glClear( GL_COLOR_BUFFER_BIT );

			m_blurShader->setUniformValue( "uHorizontal", horizontal );
			glActiveTexture( GL_TEXTURE0 + PP_UNIT0 );
			glBindTexture( GL_TEXTURE_2D, firstPass ? m_brightColorTex : m_pingPongTex[horizontal ? 0 : 1] );
			m_blurShader->setUniformValue( "uImage", PP_UNIT0 );

			glDrawArrays( GL_TRIANGLES, 0, 3 );

			horizontal = !horizontal;
			firstPass  = false;
		}

		m_blurShader->release();
	}

	// Step 3: Composite — combine scene + bloom, add vignette + grain
	{
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		glViewport( 0, 0, pw, ph );
		glClear( GL_COLOR_BUFFER_BIT );
		glDisable( GL_DEPTH_TEST );

		m_postProcessShader->bind();

		glActiveTexture( GL_TEXTURE0 + PP_UNIT0 );
		glBindTexture( GL_TEXTURE_2D, m_sceneColorTex );
		m_postProcessShader->setUniformValue( "uSceneColor", PP_UNIT0 );

		glActiveTexture( GL_TEXTURE0 + PP_UNIT1 );
		glBindTexture( GL_TEXTURE_2D, m_pingPongTex[0] );
		m_postProcessShader->setUniformValue( "uBloomBlur", PP_UNIT1 );

		m_postProcessShader->setUniformValue( "uBloomStrength", m_bloomStrength );
		// Dynamic vignette: stronger edge darkening at night
		float vignetteNight = m_vignetteStrength + ( 1.0f - m_daylight ) * 0.3f;
		m_postProcessShader->setUniformValue( "uVignetteStrength", vignetteNight );
		m_postProcessShader->setUniformValue( "uGrainStrength", m_grainStrength );
		m_postProcessShader->setUniformValue( "uTime", (float)GameState::tick );

		glDrawArrays( GL_TRIANGLES, 0, 3 );

		m_postProcessShader->release();
		glEnable( GL_DEPTH_TEST );
	}

	// Clean up — unbind post-process textures and restore VAO
	glActiveTexture( GL_TEXTURE0 + PP_UNIT0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glActiveTexture( GL_TEXTURE0 + PP_UNIT1 );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindVertexArray( 0 );
	glActiveTexture( GL_TEXTURE0 );
}

void MainWindowRenderer::onTileUpdates( const TileDataUpdateInfo& updates )
{
	m_pendingUpdates.push_back( updates.updates );
	emit redrawRequired();
}

void MainWindowRenderer::onThoughtBubbles( const ThoughtBubbleInfo& bubbles )
{
	m_thoughBubbles = bubbles;
	emit redrawRequired();
}

void MainWindowRenderer::onAxelData( const AxleDataInfo& data )
{
	m_axleData = data;
	emit redrawRequired();
}

QString MainWindowRenderer::copyShaderToString( QString name )
{
	QFile file( Global::cfg->get( "dataPath" ).toString() + "/shaders/" + name + ".glsl" );
	file.open( QIODevice::ReadOnly );
	QTextStream in( &file );
	QString code( "" );
	while ( !in.atEnd() )
	{
		code += in.readLine();
		code += "\n";
	}

	return code;
}

QOpenGLShaderProgram* MainWindowRenderer::initShader( QString name )
{
	QString vs = copyShaderToString( name + "_v" );
	QString fs = copyShaderToString( name + "_f" );

	QScopedPointer<QOpenGLShaderProgram> shader(new QOpenGLShaderProgram);

	bool ok = true;
	ok &= shader->addShaderFromSourceCode( QOpenGLShader::Vertex, vs );
	ok &= shader->addShaderFromSourceCode( QOpenGLShader::Fragment, fs );
	if ( !ok )
	{
		qCritical() << "failed to add shader source code" << name;
		return nullptr;
	}

	ok &= shader->link();

	if ( !ok )
	{
		qCritical() << "failed to link shader" << name;
		return nullptr;
	}

	ok &= shader->bind();
	if ( !ok )
	{
		qCritical() << "failed to bind shader" << name;
		return nullptr;
	}

	shader->release();

	return shader.take();
}

bool MainWindowRenderer::initShaders()
{
	m_worldShader.reset(initShader( "world" ));
	m_thoughtBubbleShader.reset(initShader( "thoughtbubble" ));
	m_selectionShader.reset(initShader( "selection" ));
	m_axleShader.reset(initShader( "axle" ));
	m_postProcessShader.reset(initShader( "postprocess" ));
	m_brightExtractShader.reset(initShader( "brightextract" ));
	m_blurShader.reset(initShader( "blur" ));

	if ( !m_worldShader || !m_thoughtBubbleShader || !m_selectionShader || !m_axleShader
		|| !m_postProcessShader || !m_brightExtractShader || !m_blurShader )
	{
		// Can't proceed, and need to know what happened!
		abort();
		return false;
	}

	m_reloadShaders = false;

	return true;
}

void MainWindowRenderer::createArrayTexture( int unit, int depth )
{
	//GLint max_layers;
	//glGetIntegerv ( GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers );

	glActiveTexture( GL_TEXTURE0 + unit );
	glGenTextures( 1, &m_textures[unit] );
	glBindTexture( GL_TEXTURE_2D_ARRAY, m_textures[unit] );
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY,
		0,             // Mip level 0
		GL_RGBA8,      // Internal format
		32, 64,        // width, height
		depth,         // Number of layers
		0,             // Border
		GL_RGBA,       // Format
		GL_UNSIGNED_BYTE,
		nullptr        // No initial data
	);
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0 );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

void MainWindowRenderer::uploadArrayTexture( int unit, int depth, const uint8_t* data )
{
	glActiveTexture( GL_TEXTURE0 + unit );
	glTexSubImage3D(
		GL_TEXTURE_2D_ARRAY,
		0,                // Mipmap number
		0, 0, 0,          // xoffset, yoffset, zoffset
		32, 64, depth,    // width, height, depth
		GL_RGBA,          // format
		GL_UNSIGNED_BYTE, // type
		data
	);
}

void MainWindowRenderer::initTextures()
{
	GLint max_layers;
	glGetIntegerv( GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers );

	qDebug() << "max array size: " << max_layers;
	qDebug() << "used " << Global::eventConnector->game()->sf()->size() << " sprites";

	int maxArrayTextures = Global::cfg->get( "MaxArrayTextures" ).toInt();

	for ( int i = 0; i < 16; ++i )
	{
		createArrayTexture( i, maxArrayTextures );
	}

	m_texesInitialized = true;
}

void MainWindowRenderer::initWorld()
{
	QElapsedTimer timer;
	timer.start();

	GLsizeiptr bufferSize = TD_SIZE * sizeof( unsigned int ) * Global::eventConnector->game()->w()->world().size();

	glGenBuffers( 1, &m_tileBo );
	glBindBuffer( GL_TEXTURE_BUFFER, m_tileBo );
	glBufferData( GL_TEXTURE_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW );
	// Zero-fill the buffer
	void* ptr = glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY );
	if ( ptr )
	{
		memset( ptr, 0, bufferSize );
		glUnmapBuffer( GL_TEXTURE_BUFFER );
	}
	glBindBuffer( GL_TEXTURE_BUFFER, 0 );

	// Create texture buffer object to access tile data from shaders
	glGenTextures( 1, &m_tileTbo );
	glActiveTexture( GL_TEXTURE0 + 16 );
	glBindTexture( GL_TEXTURE_BUFFER, m_tileTbo );
	glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_tileBo );

	m_texesInitialized = true;

	m_rotation = 0;

	emit fullDataRequired();
}

void MainWindowRenderer::updateRenderParams()
{
	m_renderSize = qMin( Global::dimX, (int)( ( sqrt( m_width * m_width + m_height * m_height ) / 12 ) / m_scale ) );

	m_renderDepth = Global::cfg->get( "renderDepth" ).toInt();

	m_viewLevel = GameState::viewLevel;

	m_volume.min = { 0, 0, qMin( qMax( m_viewLevel - m_renderDepth, 0 ), Global::dimZ - 1 ) };
	m_volume.max = { Global::dimX - 1, Global::dimY - 1, qMin( m_viewLevel, Global::dimZ - 1 ) };

	m_lightMin = Global::cfg->get( "lightMin" ).toFloat();
	if ( m_lightMin < 0.01 )
		m_lightMin = 0.06f;

	m_debug   = Global::debugMode;

	m_projectionMatrix.setToIdentity();
	m_projectionMatrix.ortho( -m_width / 2, m_width / 2, -m_height / 2, m_height / 2, -( m_volume.max.x + m_volume.max.y + m_volume.max.z + 1 ), -m_volume.min.z );
	m_projectionMatrix.scale( m_scale, m_scale );
	m_projectionMatrix.translate( m_moveX, -m_moveY );

	/*
	QString msg = "Move: " + QString::number( m_moveX ) + ", " + QString::number( m_moveY ) + " z-Level: " + QString::number( m_viewLevel ); 
	qDebug() << msg;
	//emit sendOverlayMessage( 5, msg );
	msg = "Window size: " + QString::number( m_width ) + "x" + QString::number( m_height ) + " Scale " + QString::number( m_scale ) + " Rotation " + QString::number( m_rotation );
	qDebug() << msg;
	//emit sendOverlayMessage( 4, msg );
	*/
}

void MainWindowRenderer::paintWorld()
{
	DebugScope s( "paint world" );
	QElapsedTimer timer;
	//timer.start();
	{
		DebugScope s( "clear" );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glDisable( GL_STENCIL_TEST );
		glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
		glDisable( GL_SCISSOR_TEST );

		glDepthMask( true );
		glStencilMask( 0xFFFFFFFF );
		glClearStencil( 0 );
		glClearDepth( 1 );
		glClearColor( 0.0, 0.0, 0.0, 1.0 );
		glColorMask( true, true, true, true );
		//glClearDepth( 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}

	if ( m_inMenu )
	{
		return;
	}

	if ( m_reloadShaders )
	{
		DebugScope s( "init shaders" );
		if ( !initShaders() )
		{
			return;
		}
	}

	if ( !m_texesInitialized )
	{
		DebugScope s( "init textures" );
		initWorld();
		initTextures();
		updateRenderParams();
	}

	// Initialize post-process FBOs on first real render
	if ( !m_postProcessReady && m_width > 0 && m_height > 0 )
	{
		m_dpr = m_parent->devicePixelRatioF();
		initPostProcess();
	}

	updateWorld();

	// Rebind correct textures to texture units
	for ( auto unit = 0; unit < 16; ++unit )
	{
		glActiveTexture( GL_TEXTURE0 + unit );
		glBindTexture( GL_TEXTURE_2D_ARRAY, m_textures[unit] );
	}

	timer.start();
	updateTextures();

	QString msg = "render time: " + QString::number( timer.elapsed() ) + " ms";
	//emit sendOverlayMessage( 1, msg );

	// Render world to default framebuffer first (avoids FBO rendering issues on macOS)
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glDisable( GL_STENCIL_TEST );

		glDepthMask( true );
		glStencilMask( 0xFFFFFFFF );
		glClearStencil( 0 );
		glClearDepth( 1 );
		glClearColor( 0.0, 0.0, 0.0, 1.0 );
		glColorMask( true, true, true, true );
		//glClearDepth( 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

		QOpenGLVertexArrayObject::Binder vaoBinder( &m_vao );

		paintTiles();

		paintSelection();

		paintThoughtBubbles();

		if ( Global::showAxles )
		{
			paintAxles();
		}
	}

	// Post-processing: copy rendered scene to FBO, apply bloom + vignette + grain
	if ( m_postProcessReady )
	{
		int pw = m_width * m_dpr;
		int ph = m_height * m_dpr;

		// Copy rendered scene from default FB to our scene color texture
		glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_sceneFbo );
		glBlitFramebuffer( 0, 0, pw, ph, 0, 0, pw, ph,
			GL_COLOR_BUFFER_BIT, GL_NEAREST );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );

		renderPostProcess();
	}

	//glFinish();

	bool pause = Global::cfg->get( "Pause" ).toBool();

	if ( pause != m_pause )
	{
		m_pause = pause;
	}
}

void MainWindowRenderer::onRenderParamsChanged()
{
	updateRenderParams();
	emit redrawRequired();
	emit signalCameraPosition(m_moveX, m_moveY, m_viewLevel, m_rotation, m_scale);
	
}

void MainWindowRenderer::setCommonUniforms( QOpenGLShaderProgram* shader )
{
	auto indexTotal = shader->uniformLocation( "uWorldSize" );
	if ( indexTotal >= 0 )
	{
		glUniform3ui( indexTotal, Global::dimX, Global::dimY, Global::dimZ );
	}
	auto indexMin = shader->uniformLocation( "uRenderMin" );
	if ( indexMin >= 0 )
	{
		glUniform3ui( indexMin, m_volume.min.x, m_volume.min.y, m_volume.min.z );
	}
	auto indexMax = shader->uniformLocation( "uRenderMax" );
	if ( indexMax >= 0 )
	{
		glUniform3ui( indexMax, m_volume.max.x, m_volume.max.y, m_volume.max.z );
	}
	shader->setUniformValue( "uTransform", m_projectionMatrix );
	shader->setUniformValue( "uWorldRotation", (GLuint)m_rotation );
	shader->setUniformValue( "uTickNumber", (GLuint)GameState::tick );
}

void MainWindowRenderer::paintTiles()
{
	DebugScope s( "paint tiles" );

	m_worldShader->bind();
	setCommonUniforms( m_worldShader.get() );

	// Bind tile data TBO
	glActiveTexture( GL_TEXTURE0 + 16 );
	glBindTexture( GL_TEXTURE_BUFFER, m_tileTbo );
	m_worldShader->setUniformValue( "uTileData", 16 );

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		auto texNum = "uTexture[" + QString::number( i ) + "]";
		m_worldShader->setUniformValue( texNum.toStdString().c_str(), i );
	}

	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	m_worldShader->setUniformValue( "uOverlay", Global::showDesignations );
	m_worldShader->setUniformValue( "uShowJobs", Global::showJobs );
	m_worldShader->setUniformValue( "uDebug", m_debug );
	m_worldShader->setUniformValue( "uWallsLowered", Global::wallsLowered );

	m_worldShader->setUniformValue( "uUndiscoveredTex", Global::undiscoveredUID * 4 );
	m_worldShader->setUniformValue( "uWaterTex", Global::waterSpriteUID * 4 );

	// Smooth daylight curve based on time of day
	{
		float timeNorm = ( GameState::hour * 60.0f + GameState::minute ) / 1440.0f; // 0-1 over 24h
		float sunriseNorm = GameState::sunrise / 1440.0f;
		float sunsetNorm  = GameState::sunset / 1440.0f;
		float moonlight = 0.08f; // ambient moonlight at night — darkness should feel threatening

		float target;
		float dawnWidth = 0.04f; // ~1 hour transition

		if ( timeNorm < sunriseNorm - dawnWidth )
			target = moonlight;
		else if ( timeNorm < sunriseNorm + dawnWidth )
		{
			float t = ( timeNorm - sunriseNorm + dawnWidth ) / ( dawnWidth * 2 );
			target = moonlight + ( 1.0f - moonlight ) * t;
		}
		else if ( timeNorm < sunsetNorm - dawnWidth )
			target = 1.0f;
		else if ( timeNorm < sunsetNorm + dawnWidth )
		{
			float t = ( timeNorm - sunsetNorm + dawnWidth ) / ( dawnWidth * 2 );
			target = 1.0f - ( 1.0f - moonlight ) * t;
		}
		else
			target = moonlight;

		m_daylight = m_daylight + ( target - m_daylight ) * 0.05; // smooth lerp
	}
	m_worldShader->setUniformValue( "uDaylight", (float)m_daylight );
	m_worldShader->setUniformValue( "uLightMin", m_lightMin );
	m_worldShader->setUniformValue( "uViewLevel", (float)m_viewLevel );
	m_worldShader->setUniformValue( "uRenderDepth", (float)m_renderDepth );
	m_worldShader->setUniformValue( "uSeason", (GLint)GameState::season );

	// Weather type mapping
	{
		int weatherType = 0;
		if ( GameState::activeWeather == "Storm" )
			weatherType = 1;
		else if ( GameState::activeWeather == "HeatWave" )
			weatherType = 2;
		else if ( GameState::activeWeather == "ColdSnap" )
			weatherType = 3;

		// Smooth ramp weather intensity
		float targetIntensity = ( weatherType != 0 ) ? 1.0f : 0.0f;
		m_weatherIntensity = m_weatherIntensity + ( targetIntensity - m_weatherIntensity ) * 0.03f;

		m_worldShader->setUniformValue( "uWeather", (GLint)weatherType );
		m_worldShader->setUniformValue( "uWeatherIntensity", m_weatherIntensity );
	}

	auto volume   = m_volume.size();
	GLsizei tiles = volume.x * volume.y * volume.z;

	// First pass is pure front-to-back of opaque blocks, alpha doesn't even work
	glDisable( GL_BLEND );
	glDepthMask( true );

	m_worldShader->setUniformValue( "uPaintFrontToBack", true );
	glDrawElementsInstanced( GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, 0, tiles );

	//!TODO Transparency pass is too early, all the stuff rendered later is still missing
	// Second pass includes transparency
	m_worldShader->setUniformValue( "uPaintFrontToBack", false );
	glEnable( GL_BLEND );
	glDrawElementsInstanced( GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, (void*)( sizeof( GLushort ) * 6 ), tiles );

	// All done with depth writes, everything beyond is layered
	glDepthMask( false );

	m_worldShader->release();
}

void MainWindowRenderer::paintSelection()
{
	if ( m_selectionData.isEmpty() )
	{
		return;
	}

	// TODO this is a workaround until some transparency solution is implemented
	if( m_selectionNoDepthTest )
	{
		glDisable( GL_DEPTH_TEST );
	}
	DebugScope s( "paint selection" );
	m_selectionShader->bind();
	setCommonUniforms( m_selectionShader.get() );

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		auto texNum = "uTexture[" + QString::number( i ) + "]";
		m_selectionShader->setUniformValue( texNum.toStdString().c_str(), i );
	}

	GLint tileLoc = m_selectionShader->uniformLocation( "tile" );
	for ( const auto& sd : m_selectionData )
	{
		glUniform3ui( tileLoc, sd.pos.x, sd.pos.y, sd.pos.z );
		m_selectionShader->setUniformValue( "uSpriteID", sd.spriteID );
		m_selectionShader->setUniformValue( "uRotation", sd.localRot );
		m_selectionShader->setUniformValue( "uValid", sd.valid );

		glDrawArraysInstanced( GL_TRIANGLE_STRIP, 0, 4, 1 );
	}

	m_selectionShader->release();
	if( m_selectionNoDepthTest )
	{
		glEnable( GL_DEPTH_TEST );
	}
}

void MainWindowRenderer::paintThoughtBubbles()
{
	DebugScope s( "paint thoughts" );

	m_thoughtBubbleShader->bind();
	setCommonUniforms( m_thoughtBubbleShader.get() );

	m_thoughtBubbleShader->setUniformValue( "uTexture0", 0 );

	for ( const auto& thoughtBubble : m_thoughBubbles.thoughtBubbles )
	{
		if ( thoughtBubble.pos.z <= m_viewLevel )
		{
			GLint tile = m_axleShader->uniformLocation( "tile" );
			glUniform3ui( tile, thoughtBubble.pos.x, thoughtBubble.pos.y, thoughtBubble.pos.z );
			m_thoughtBubbleShader->setUniformValue( "uType", thoughtBubble.sprite );
			glDrawArraysInstanced( GL_TRIANGLE_STRIP, 0, 4, 1 );
		}
	}

	m_thoughtBubbleShader->release();
}

void MainWindowRenderer::paintAxles()
{
	DebugScope s( "paint axles" );

	m_axleShader->bind();
	setCommonUniforms( m_axleShader.get() );
	m_axleShader->setUniformValue( "uTickNumber", (unsigned int)GameState::tick );

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		auto texNum = "uTexture[" + QString::number( i ) + "]";
		m_axleShader->setUniformValue( texNum.toStdString().c_str(), i );
	}

	for ( const auto& ad : m_axleData.data )
	{
		if ( !ad.isVertical && ad.pos.z <= m_viewLevel )
		{
			GLint tile = m_axleShader->uniformLocation( "tile" );
			glUniform3ui( tile, ad.pos.x, ad.pos.y, ad.pos.z );
			m_axleShader->setUniformValue( "uSpriteID", ad.spriteID );
			m_axleShader->setUniformValue( "uRotation", ad.localRot );
			m_axleShader->setUniformValue( "uAnim", ad.anim );

			glDrawArraysInstanced( GL_TRIANGLE_STRIP, 0, 4, 1 );
		}
	}

	m_axleShader->release();
}

void MainWindowRenderer::resize( int w, int h )
{
	m_width  = w;
	m_height = h;
	m_dpr    = m_parent->devicePixelRatioF();
	if ( m_postProcessReady )
	{
		resizePostProcess( w, h );
	}
	onRenderParamsChanged();
}

void MainWindowRenderer::rotate( int direction )
{
	direction  = qBound( -1, direction, 1 );
	m_rotation = ( 4 + m_rotation + direction ) % 4;
	
	if( direction == 1 )
	{
		updatePositionAfterCWRotation( m_moveX, m_moveY );
	}
	else
	{
		updatePositionAfterCWRotation( m_moveX, m_moveY );
		updatePositionAfterCWRotation( m_moveX, m_moveY );
		updatePositionAfterCWRotation( m_moveX, m_moveY );
	}
	onRenderParamsChanged();
}

void MainWindowRenderer::move( float x, float y )
{
	if ( !Global::dimX )
		return;

	m_moveX += x / m_scale;
	m_moveY += y / m_scale;

	const auto centerY = -Global::dimX * 8.f;
	const auto centerX = 0;

	float oldX, oldY;
	do
	{
		oldX   = m_moveX;
		oldY   = m_moveY;
		const auto rangeY = Global::dimX * 8.f - abs( m_moveX - centerX ) / 2.f;
		const auto rangeX = Global::dimX * 16.f - abs( m_moveY - centerY ) * 2.f;
		m_moveX           = qBound( centerX - rangeX, m_moveX, centerX + rangeX );
		m_moveY           = qBound( centerY - rangeY, m_moveY, centerY + rangeY );
	} while ( oldX != m_moveX || oldY != m_moveY );

	GameState::moveX = m_moveX;
	GameState::moveY = m_moveY;
	
	onRenderParamsChanged();
}

void MainWindowRenderer::scale( float factor )
{
	m_scale *= factor;
	m_scale = qBound( 0.25f, m_scale, 15.f );
	GameState::scale = m_scale;
	onRenderParamsChanged();
}

void MainWindowRenderer::setScale( float scale )
{
	m_scale = qBound( 0.25f, scale, 15.f );
	onRenderParamsChanged();
}

void MainWindowRenderer::setViewLevel( int level )
{
	m_viewLevel = level;
	onRenderParamsChanged();
}

void MainWindowRenderer::updateWorld()
{
	if ( !m_pendingUpdates.empty() )
	{
		DebugScope s( "update world" );

		// Count total updates to choose upload strategy
		int totalUpdates = 0;
		for ( const auto& batch : m_pendingUpdates )
		{
			totalUpdates += batch.size();
		}

		if ( totalUpdates > 10000 )
		{
			// Bulk upload: map the entire buffer once and write all tiles
			uploadTileDataBulk( m_pendingUpdates );
		}
		else
		{
			// Small update: use per-tile glBufferSubData
			for ( const auto& batch : m_pendingUpdates )
			{
				uploadTileData( batch );
			}
		}
		m_pendingUpdates.clear();
	}
}

void MainWindowRenderer::uploadTileData( const QVector<TileDataUpdate>& tileData )
{
	glBindBuffer( GL_TEXTURE_BUFFER, m_tileBo );
	for ( const auto& update : tileData )
	{
		GLintptr offset = update.id * TD_SIZE * sizeof( unsigned int );
		glBufferSubData( GL_TEXTURE_BUFFER, offset, sizeof( TileData ), &update.tile );
	}
	glBindBuffer( GL_TEXTURE_BUFFER, 0 );
}

void MainWindowRenderer::uploadTileDataBulk( const QVector<QVector<TileDataUpdate>>& allUpdates )
{
	glBindBuffer( GL_TEXTURE_BUFFER, m_tileBo );
	unsigned char* ptr = (unsigned char*)glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY );
	if ( ptr )
	{
		for ( const auto& batch : allUpdates )
		{
			for ( const auto& update : batch )
			{
				size_t offset = update.id * TD_SIZE * sizeof( unsigned int );
				memcpy( ptr + offset, &update.tile, sizeof( TileData ) );
			}
		}
		glUnmapBuffer( GL_TEXTURE_BUFFER );
	}
	else
	{
		// Fallback to per-tile upload
		for ( const auto& batch : allUpdates )
		{
			uploadTileData( batch );
		}
	}
	glBindBuffer( GL_TEXTURE_BUFFER, 0 );
}

void MainWindowRenderer::updateTextures()
{
	if ( Global::eventConnector->game()->sf()->textureAdded() || Global::eventConnector->game()->sf()->creatureTextureAdded() )
	{
		DebugScope s( "update textures" );

		m_texesUsed = Global::eventConnector->game()->sf()->texesUsed();

		int maxArrayTextures = Global::cfg->get( "MaxArrayTextures" ).toInt();

		for ( int i = 0; i < m_texesUsed; ++i )
		{
			uploadArrayTexture( i, maxArrayTextures, Global::eventConnector->game()->sf()->pixelData( i ).cbegin() );
		}
	}
}

void MainWindowRenderer::onUpdateSelection( const QMap<unsigned int, SelectionData>& data, bool noDepthTest )
{
	m_selectionData.clear();
	for( const auto& key : data.keys() )
	{
		m_selectionData.insert( key, data[key] );
	}
	m_selectionNoDepthTest = noDepthTest;
	emit redrawRequired();
}

void MainWindowRenderer::onCenterCameraPosition( const Position& target )
{
	m_moveX     = 16 * (-target.x + target.y);
	m_moveY     = 8 * ( -target.x - target.y );
	m_viewLevel = target.z;
	onRenderParamsChanged();
}

void MainWindowRenderer::updatePositionAfterCWRotation( float& x, float& y )
{
	constexpr int tileHeight = 8; //tiles are assumed to be 8 pixels high (and twice as wide)
	int tmp = x;
	x = -2 * ( Global::dimX * tileHeight + y );
	y = -( Global::dimY - 2 ) * tileHeight + tmp/2;
}

void MainWindowRenderer::onSetInMenu( bool value )
{
	m_inMenu = value;
	emit redrawRequired();
}