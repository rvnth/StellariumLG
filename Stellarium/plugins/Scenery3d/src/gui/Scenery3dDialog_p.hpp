/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _SCENERY3DDIALOG_P_HPP_
#define _SCENERY3DDIALOG_P_HPP_

#include "S3DEnum.hpp"
#include "Scenery3dMgr.hpp"

#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

#include <QAbstractListModel>

class CubemapModeListModel : public QAbstractListModel
{
	Q_OBJECT
private:
	Scenery3dMgr* mgr;
public:


	CubemapModeListModel(QObject* parent = NULL) : QAbstractListModel(parent)
	{
		mgr = GETSTELMODULE(Scenery3dMgr);
		Q_ASSERT(mgr);
	}

	int rowCount(const QModelIndex &parent) const
	{
		Q_UNUSED(parent);
		if(mgr->getIsANGLE())
		{
			return 1;
		}
		if(!mgr->getIsGeometryShaderSupported())
		{
			return 2;
		}
		return 3;
	}

	QVariant data(const QModelIndex &index, int role) const
	{
		if(role == Qt::DisplayRole || role == Qt::EditRole)
		{
			switch (index.row())
			{
				case S3DEnum::CM_TEXTURES:
					return QVariant(QString(q_("6 Textures")));
				case S3DEnum::CM_CUBEMAP:
					return QVariant(QString(q_("Cubemap")));
				case S3DEnum::CM_CUBEMAP_GSACCEL:
					return QVariant(QString(q_("Geometry shader")));
			}
		}
		return QVariant();
	}
};


#endif