/***************************************************************************
 *   Copyright (C) 2005-2013 by the FIFE team                              *
 *   http://www.fifengine.net                                              *
 *   This file is part of FIFE.                                            *
 *                                                                         *
 *   FIFE is free software; you can redistribute it and/or                 *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef FIFE_GUI_WIDGETS_PANEL_H
#define FIFE_GUI_WIDGETS_PANEL_H

// Standard C++ library includes

// 3rd party library includes
#include <fifechan.hpp>

// FIFE includes
// These includes are split up in two parts, separated by one empty line
// First block: files included from the FIFE root src directory
// Second block: files included from the same folder
#include "resizablewindow.h"

namespace fcn {
	class DockArea;
	class Panel : public ResizableWindow {
	public:
		Panel();
		Panel(bool dockable);

		virtual ~Panel();
        
		/** Sets if the widget is dockable.
		 * @param dockable True if the widget should be dockable, otherwise false.
		 */
		void setDockable(bool dockable);

		/** Gets if the widget is dockable.
		 * @return True if the widget should be dockable, otherwise false.
		 */
		bool isDockable() const;

		/** Sets if the widget is docked.
		 * @param docked True if the widget is docked, otherwise false.
		 */
		void setDocked(bool docked);

		/** Gets if the widget is docked.
		 * @return True if the widget is docked, otherwise false.
		 */
		bool isDocked() const;

		// Inherited from ResizableWindow

		virtual void resizeToContent(bool recursiv=true);
		virtual void expandContent(bool recursiv=true);

		// Inherited from ResizableWindow / FocusListener
	
		virtual void focusLost(const Event& event);
	
	
		// Inherited from ResizableWindow / MouseListener

		virtual void mouseEntered(MouseEvent& mouseEvent);

		virtual void mouseExited(MouseEvent& mouseEvent);

		virtual void mousePressed(MouseEvent& mouseEvent);

		virtual void mouseReleased(MouseEvent& mouseEvent);

		virtual void mouseMoved(MouseEvent& mouseEvent);
	
		virtual void mouseDragged(MouseEvent& mouseEvent);

	protected:
		void expand(bool expand);
		void changeLayout();
		// resets the prepared states
		void resetPrepared();
		void findDockArea();

		// is dockable
		bool m_dockable;
		// is docked
		bool m_docked;
		// is expanded
		bool m_expanded;

		// indicate the prepared states
		bool m_preDock;
		bool m_preUndock;
		bool m_preExpand;
		bool m_preLayout;
		// first Dock Area that was found while the Panel was dragged
		DockArea* m_foundDockArea;
		// Dock Area this Panel is docked to
		DockArea* m_dockedArea;
		Container* m_origParent;

		struct SavedState	{
			Rectangle dimension;
			uint32_t innerBorder;
			bool resizable;
			bool movable;
		};
		SavedState m_state;
	};
}

#endif
/* vim: set noexpandtab: set shiftwidth=2: set tabstop=2: */
