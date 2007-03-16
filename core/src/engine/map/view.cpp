/***************************************************************************
 *   Copyright (C) 2005-2007 by the FIFE Team                              *
 *   fife-public@lists.sourceforge.net                                     *
 *   This file is part of FIFE.                                            *
 *                                                                         *
 *   FIFE is free software; you can redistribute it and/or modify          *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA              *
 ***************************************************************************/

// Standard C++ library includes
#include <algorithm>
#include <cmath>

// 3rd party library includes
#include <SDL.h>

// FIFE includes
// These includes are split up in two parts, separated by one empty line
// First block: files included from the FIFE root src directory
// Second block: files included from the same folder
#include "map/effects/fade.h"
#include "video/animation.h"
#include "video/renderable.h"
#include "video/screen.h"
#include "debugutils.h"
#include "exception.h"
#include "imagecache.h"
#include "log.h"

#include "camera.h"
#include "elevation.h"
#include "geometry.h"
#include "layer.h"
#include "map.h"
#include "objectinfo.h"
#include "view.h"
#include "visual.h"
#include "visualtree.h"

namespace FIFE { namespace map {

	View::View() 
		:
		m_vtree(new VisualTree()),
		m_surface(0),
		m_rect(),
		m_elevation(),
		m_layer_pos(-1) {
		// Quick hack for getting the masks.
		// msef003.frm >> hex outline
		// msef000.frm >> hex outline
		m_layer_id = 0;
		m_tilemask = ImageCache::instance()->addImageFromFile("content/gfx/tiles/tile_outline.png");
		// m_objmask =  ImageCache::instance()->addImageFromFile("msef003.frm");

	}

	View::~View() {
		reset();
		delete m_vtree;
	}

	View::View(const View&) 
		: m_surface(0),
		m_rect(),
		m_elevation(),
		m_layer_pos() , m_layer_id(0) {
	}

	View& View::operator=(const View&) {
		return *this;
	}

	void View::setViewport(Screen* surface) {
		if( !surface ) {
			// Here I assume that the call setViewport(0) in ViewGameState::deactivate()
			// was made on purpose! -> Inhibit all calls in render()
			m_surface = 0;
			return;
		}

		int w = surface->getWidth();
		int h = surface->getHeight();

		setViewport(surface, Rect(w/4, h/4, w/2, h/2));
	}

	void View::setViewport(Screen* surface, const Rect& rect) {
		m_surface = surface;
		m_rect = rect;
	}

	void View::setMap(MapPtr map, size_t elev) {
		m_map = map;
		m_elevation = map->getElevation( elev );
		m_elevation_id = elev;
// 		m_camera->reset(this);
		elevationChange();
	}

	void View::setXPos(unsigned int pos) {
		m_offset.x = pos;
	}

	void View::setYPos(unsigned int pos) {
		m_offset.y = pos;
	}

	int View::getXPos() const {
		return m_offset.x;
	}

	int View::getYPos() const {
		return m_offset.y;
	}

	size_t View::addVisual( Visual* visual ) {
		if( visual == 0 ) {
			return 0;
		}
		visual->reset();
// 		Log("map_view")
// 			<< "Adding visual at " << visual->getScreenBox();
		return m_vtree->addVisual(visual);
	}

	void View::removeVisual( size_t visualId ) {
		m_vtree->removeVisual(visualId);
	}

	Visual* View::getVisual(size_t id) {
		return m_vtree->getVisual(id);
	}

	bool View::isValidVisualId(size_t id) const {
		return m_vtree->isValidVisualId(id);
	}

	void View::notifyVisualChanged(size_t visualId) {
		Visual *visual = m_vtree->getVisual(visualId);
		if( !visual )
			return;
		visual->reset();
		m_vtree->updateVisual( visualId );
	}
	void View::elevationChange() {
		clearVisuals();

	}

	void View::clearVisuals() {
		m_vtree->clear();
	}

	void View::loadOverlayImage(const std::string & filename) {
		m_layer_id = ImageCache::instance()->addImageFromFile(filename);
		//m_layer_pos = -1;
	}

	void View::renderGridOverlay(LayerPtr layer ) {
		Point offset = m_offset + layer->getShift();
		Geometry* geometry = layer->getGeometry();

		int startx, starty, stopx, stopy;
		Rect draw = geometry->gridBoundingRect(Rect(offset.x,offset.y,m_rect.w,m_rect.h));
		startx = draw.x;
		starty = draw.y;
		int x_range = draw.w;
		int y_range = draw.h;
		Point size = layer->getSize();
		stopx = startx + x_range + 2;
		stopx = std::min(size.x, stopx);
		startx = std::max(0, startx);
		stopy = starty + y_range + 2;
		starty = std::max(0,starty);
		stopy = std::min(size.y,stopy);

		Point basesize = geometry->baseSize();
		size_t image_id = getGridOverlayImageId( layer );
		RenderAble* renderable = ImageCache::instance()->getImage( image_id );

		offset.x -= m_rect.x;
		offset.y -= m_rect.y;

		Point index;
		for (index.y = starty; index.y < stopy; ++index.y) {
			for (index.x = startx; index.x < stopx; ++index.x) {
				Point pos = geometry->toScreen(index) - offset;
				Rect target(pos.x,pos.y,basesize.x,basesize.y);
				renderable->render(target,m_surface);
			}
		}
	}


	void View::renderTiles( LayerPtr layer ) {
		Point offset = m_offset + layer->getShift();
		Geometry* geometry = layer->getGeometry();

		int startx, starty, stopx, stopy;
		Rect draw = geometry->gridBoundingRect(Rect(offset.x,offset.y,m_rect.w,m_rect.h));
		startx = draw.x;
		starty = draw.y;
		int x_range = draw.w;
		int y_range = draw.h;
		Point size = layer->getSize();
		stopx = startx + x_range + 2;
		stopx = std::min(size.x, stopx);
		startx = std::max(0, startx - 1);
		stopy = starty + y_range + 2;
		starty = std::max(0,starty - 1);
		stopy = std::min(size.y,stopy);

		offset.x -= m_rect.x;
		offset.y -= m_rect.y;

		Point basesize = geometry->baseSize();
		uint8_t alpha = layer->getGlobalAlpha();
		Point index;
		for (index.y = starty; index.y < stopy; ++index.y) {
			for (index.x = startx; index.x < stopx; ++index.x) {
				Point pos = geometry->toScreen(index) - offset;
				Rect target(pos.x,pos.y,basesize.x,basesize.y);
				size_t id = layer->getTileImage(index);
				RenderAble* renderable = ImageCache::instance()->getImage(id);
				renderable->render(target,m_surface,alpha);
			}
		}

	}

	void View::renderLayerOverlay(LayerPtr layer, const Point& pos) {
		Geometry *geometry = layer->getGeometry();
		Point overlay_offset = layer->get<Point>("_OVERLAY_IMAGE_OFFSET");

		size_t image_id = getGridOverlayImageId( layer );

		RenderAble* image = ImageCache::instance()->getImage( image_id );

		Point basesize = geometry->baseSize();
		Point spos = geometry->toScreen(pos) - m_offset - overlay_offset;
		Rect target(spos.x,spos.y,basesize.x,basesize.y);
		image->render(target,m_surface);
	}

	size_t View::getGridOverlayImageId(LayerPtr layer) {
		// FIXME Using the metadata like this (saving the id) is crap.
		size_t image_id = layer->get<size_t>("_OVERLAY_IMAGE_ID",0);
		if( image_id == 0 ) {
			std::string overlay_image = layer->get<std::string>("_OVERLAY_IMAGE");
			image_id = ImageCache::instance()->addImageFromFile(overlay_image);
			layer->set<size_t>("_OVERLAY_IMAGE_ID",image_id);
		}
		return image_id;
	}

	void View::render() {
		const int delta_border = 20;

		if (!m_elevation || ! m_surface)
			return;

		m_surface->pushClipArea( gcn::Rectangle(m_rect.x,m_rect.y,m_rect.w,m_rect.h));
		// The viewport to search for visuals.
		// To be on the secure side, we scan a slightly larger
		// than the actual viewport.
		Rect viewport(m_rect);
		viewport.x  = m_offset.x - delta_border;
		viewport.y  = m_offset.y - delta_border;
		viewport.w += delta_border*2;
		viewport.h += delta_border*2;

		Point offset(m_offset);
		offset.x -= m_rect.x;
		offset.y -= m_rect.y;

		VisualTree::RenderList &renderlist = m_vtree->calculateRenderlist(viewport);
		VisualTree::RenderList::iterator visual_it = renderlist.begin();

		for(size_t i=0; i!= m_elevation->getNumLayers(); ++i) {
			LayerPtr layer          = m_elevation->getLayer(i);
			uint8_t  layer_alpha    = layer->getGlobalAlpha();
			bool     layer_ovisible = layer->areObjectsVisible();
			bool     render_tiles = layer->areTilesVisible() && layer->hasTiles();

			if( !layer_ovisible || layer_alpha == 0 ) {
				while( visual_it != renderlist.end() && (*visual_it)->getLayerNumber() <= i)
					++visual_it;
				continue;
			}

			if( render_tiles )
				renderTiles(layer);

			// TODO Do this correct
			Point layer_pos = i ? m_layer_pos : m_tilemask_pos;
			renderLayerOverlay(layer,layer_pos);

			if( layer->isGridOverlayEnabled() ) {
				renderGridOverlay(layer);
			}

			while( visual_it != renderlist.end() && (*visual_it)->getLayerNumber() <= i) {
				(*visual_it)->render(m_surface, offset, layer_alpha);
				++visual_it;
			}
		}

		m_surface->popClipArea();
	}


	size_t View::getElevationNum() const {
		return m_elevation_id;
	}
	size_t View::getNumElevations() const {
		assert(m_map);
		return m_map->getNumElevations();
	}

	void View::setElevationNum(size_t id) {
		m_elevation = m_map->getElevation(id);
		m_elevation_id = id;
	}

	ElevationPtr View::getCurrentElevation() const {
		return m_elevation;
	}


	void View::onMouseClick(uint8_t type, uint8_t button, unsigned short x, unsigned short y) {
		// Here's a big fat mark for deprecation
		// This should probably be somewhere else,
		// But right now it's handy for testing the viewing code

		Log("mapview") 
			<< "Click: type/button x,y " 
			<< int(type) << "/" << int(button) 
			<< " " << x << "," << y;

		if( 0 == m_map ) {
			Warn("mapview") << "Received mouse events while not initialized!";
			return;
		}

		if( m_elevation->getNumLayers() > 1 ) {
			if( button == 3 ) {
				LayerPtr layer = m_elevation->getLayer(0);
				Geometry *geometry = layer->getGeometry();
				m_tilemask_pos = geometry->fromScreen(Point(x,y) + m_offset);
				Log("mapview") << "Selected tile Layer: " << m_tilemask_pos;
			} else {
				LayerPtr layer = m_elevation->getLayer(1);
				Geometry *geometry = layer->getGeometry();
				// FIXME: I am not sure where exactly this offset comes from
				// A rounding problem in map/defaultobjectgeometry.cpp ???
				// Seems to work correctly for mouse selection, though -phoku
				m_layer_pos = geometry->fromScreen(Point(x,y) + m_offset - Point(48,16));
				Log("mapview") << "Selected object layer: " << m_layer_pos;
			}
		}

		Rect region(x-2,y-2,5,5);
		region.x += m_offset.x;
		region.y += m_offset.y;

		VisualTree::VisualRefList reflist;
		m_vtree->findVisualsAt(region,reflist);

		Log("mapview") 
			<< reflist.size() << " visuals"
			<< " touching region " << region;
		for(size_t i=0; i!= reflist.size(); ++i) {
			Log("mapview")
				<< "visual #" << reflist[i];
			Visual* visual = m_vtree->getVisual(reflist[i]);
			if( visual->getObject() )
				visual->getObject()->debugPrint();
			visual->addEffect( new effect::Fade(visual,0,250) );
// 			m_vtree->removeVisual( reflist[i] );
		}
		

	}

	void View::reset() {
		clearVisuals();
		m_map.reset();
		m_elevation.reset();
	}

} } //FIFE::map
/* vim: set noexpandtab: set shiftwidth=2: set tabstop=2: */