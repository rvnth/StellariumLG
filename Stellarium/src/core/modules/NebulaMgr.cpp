/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2011 Alexander Wolf
 * Copyright (C) 2015 Georg Zotti
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

// class used to manage groups of Nebulas

#include "StelApp.hpp"
#include "NebulaMgr.hpp"
#include "Nebula.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelSkyDrawer.hpp"
#include "StelTranslator.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelSkyImageTile.hpp"
#include "StelPainter.hpp"
#include "RefractionExtinction.hpp"
#include "StelActionMgr.hpp"

#include <algorithm>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QDir>

void NebulaMgr::setLabelsColor(const Vec3f& c) {Nebula::labelColor = c;}
const Vec3f &NebulaMgr::getLabelsColor(void) const {return Nebula::labelColor;}
void NebulaMgr::setCirclesColor(const Vec3f& c) {Nebula::circleColor = c;}
const Vec3f &NebulaMgr::getCirclesColor(void) const {return Nebula::circleColor;}
void NebulaMgr::setCircleScale(float scale) {Nebula::circleScale = scale;}
float NebulaMgr::getCircleScale(void) const {return Nebula::circleScale;}
void NebulaMgr::setHintsProportional(const bool proportional) {Nebula::drawHintProportional=proportional;}
bool NebulaMgr::getHintsProportional(void) const {return Nebula::drawHintProportional;}

NebulaMgr::NebulaMgr(void)
	: nebGrid(200),
	  hintsAmount(0),
	  labelsAmount(0)
{
	setObjectName("NebulaMgr");
}

NebulaMgr::~NebulaMgr()
{
	Nebula::texCircle = StelTextureSP();
	Nebula::texGalaxy = StelTextureSP();
	Nebula::texOpenCluster = StelTextureSP();
	Nebula::texGlobularCluster = StelTextureSP();
	Nebula::texPlanetaryNebula = StelTextureSP();
	Nebula::texDiffuseNebula = StelTextureSP();
	Nebula::texDarkNebula = StelTextureSP();
	Nebula::texOpenClusterWithNebulosity = StelTextureSP();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double NebulaMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MilkyWay")->getCallOrder(actionName)+10;
	return 0;
}

// read from stream
void NebulaMgr::init()
{
	// TODO: mechanism to specify which sets get loaded at start time.
	// candidate methods:
	// 1. config file option (list of sets to load at startup)
	// 2. load all
	// 3. flag in nebula_textures.fab (yuk)
	// 4. info.ini file in each set containing a "load at startup" item
	// For now (0.9.0), just load the default set
	loadNebulaSet("default");

	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	nebulaFont.setPixelSize(StelApp::getInstance().getBaseFontSize());
	Nebula::texCircle			= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb.png");	// Load circle texture
	Nebula::texGalaxy			= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_gal.png");	// Load ellipse texture
	Nebula::texOpenCluster			= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_ocl.png");	// Load open cluster marker texture
	Nebula::texGlobularCluster		= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_gcl.png");	// Load globular cluster marker texture
	Nebula::texPlanetaryNebula		= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_pnb.png");	// Load planetary nebula marker texture
	Nebula::texDiffuseNebula		= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_dif.png");	// Load diffuse nebula marker texture
	Nebula::texDarkNebula			= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_drk.png");	// Load dark nebula marker texture
	Nebula::texOpenClusterWithNebulosity	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_ocln.png");	// Load Ocl/Nebula marker texture
	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur5.png");   // Load pointer texture

	setFlagShow(conf->value("astro/flag_nebula",true).toBool());
	setFlagHints(conf->value("astro/flag_nebula_name",false).toBool());
	setHintsAmount(conf->value("astro/nebula_hints_amount", 3).toFloat());
	setLabelsAmount(conf->value("astro/nebula_labels_amount", 3).toFloat());
	setCircleScale(conf->value("astro/nebula_scale",1.0f).toFloat());	
	setHintsProportional(conf->value("astro/flag_nebula_hints_proportional", false).toBool());

	updateI18n();
	
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	addAction("actionShow_Nebulas", N_("Display Options"), N_("Deep-sky objects"), "flagHintDisplayed", "D", "N");
}

struct DrawNebulaFuncObject
{
	DrawNebulaFuncObject(float amaxMagHints, float amaxMagLabels, StelPainter* p, StelCore* aCore, bool acheckMaxMagHints) : maxMagHints(amaxMagHints), maxMagLabels(amaxMagLabels), sPainter(p), core(aCore), checkMaxMagHints(acheckMaxMagHints)
	{
		angularSizeLimit = 5.f/sPainter->getProjector()->getPixelPerRadAtCenter()*180.f/M_PI;
	}
	void operator()(StelRegionObject* obj)
	{
		Nebula* n = static_cast<Nebula*>(obj);
		StelSkyDrawer *drawer = core->getSkyDrawer();
		// filter out DSOs which are too dim to be seen (e.g. for bino observers)
		if ((drawer->getFlagNebulaMagnitudeLimit()) && (n->mag > drawer->getCustomNebulaMagnitudeLimit())) return;

		if (n->angularSize>angularSizeLimit || (checkMaxMagHints && n->mag <= maxMagHints))
		{
			float refmag_add=0; // value to adjust hints visibility threshold.
			sPainter->getProjector()->project(n->XYZ,n->XY);
			n->drawLabel(*sPainter, maxMagLabels-refmag_add);
			n->drawHints(*sPainter, maxMagHints -refmag_add);
		}
	}
	float maxMagHints;
	float maxMagLabels;
	StelPainter* sPainter;
	StelCore* core;
	float angularSizeLimit;
	bool checkMaxMagHints;
};

float NebulaMgr::computeMaxMagHint(const StelSkyDrawer* skyDrawer) const
{
	return skyDrawer->getLimitMagnitude()*1.2f-2.f+(hintsAmount *1.2f)-2.f;
}

// Draw all the Nebulae
void NebulaMgr::draw(StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);

	StelSkyDrawer* skyDrawer = core->getSkyDrawer();

	Nebula::hintsBrightness = hintsFader.getInterstate()*flagShow.getInterstate();

	sPainter.enableTexture2d(true);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	// Use a 1 degree margin
	const double margin = 1.*M_PI/180.*prj->getPixelPerRadAtCenter();
	const SphericalRegionP& p = prj->getViewportConvexPolygon(margin, margin);

	// Print all the nebulae of all the selected zones
	float maxMagHints  = computeMaxMagHint(skyDrawer);
	float maxMagLabels = skyDrawer->getLimitMagnitude()     -2.f+(labelsAmount*1.2f)-2.f;
	sPainter.setFont(nebulaFont);
	DrawNebulaFuncObject func(maxMagHints, maxMagLabels, &sPainter, core, hintsFader.getInterstate()>0.0001);
	nebGrid.processIntersectingPointInRegions(p.data(), func);

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, sPainter);
}

void NebulaMgr::drawPointer(const StelCore* core, StelPainter& sPainter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Nebula");	
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		// Compute 2D pos and return if outside screen
		if (!prj->projectInPlace(pos)) return;
		sPainter.setColor(0.4f,0.5f,0.8f);

		texPointer->bind();

		sPainter.enableTexture2d(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

		// Size on screen
		float size = obj->getAngularSize(core)*M_PI/180.*prj->getPixelPerRadAtCenter();

		if (Nebula::drawHintProportional)
			size*=1.2f;
		size+=20.f + 10.f*std::sin(3.f * StelApp::getInstance().getTotalRunTime());
		sPainter.drawSprite2dMode(pos[0]-size/2, pos[1]-size/2, 10, 90);
		sPainter.drawSprite2dMode(pos[0]-size/2, pos[1]+size/2, 10, 0);
		sPainter.drawSprite2dMode(pos[0]+size/2, pos[1]+size/2, 10, -90);
		sPainter.drawSprite2dMode(pos[0]+size/2, pos[1]-size/2, 10, -180);
	}
}

void NebulaMgr::setStelStyle(const QString& section)
{
	// Load colors from config file
	QSettings* conf = StelApp::getInstance().getSettings();

	QString defaultColor = conf->value(section+"/default_color").toString();
	setLabelsColor(StelUtils::strToVec3f(conf->value(section+"/nebula_label_color", defaultColor).toString()));
	setCirclesColor(StelUtils::strToVec3f(conf->value(section+"/nebula_circle_color", defaultColor).toString()));
}

// Search by name
NebulaP NebulaMgr::search(const QString& name)
{
	QString uname = name.toUpper();

	foreach (const NebulaP& n, nebArray)
	{
		QString testName = n->getEnglishName().toUpper();
		if (testName==uname) return n;
	}

	// If no match found, try search by catalog reference
	static QRegExp catNumRx("^(M|NGC|IC|C|B|VDB|RCW|LDN|LBN|CR|MEL)\\s*(\\d+)$");
	if (catNumRx.exactMatch(uname))
	{
		QString cat = catNumRx.capturedTexts().at(1);
		int num = catNumRx.capturedTexts().at(2).toInt();

		if (cat == "M") return searchM(num);
		if (cat == "NGC") return searchNGC(num);
		if (cat == "IC") return searchIC(num);
		if (cat == "C") return searchC(num);
		if (cat == "B") return searchB(num);
		if (cat == "VDB") return searchVdB(num);
		if (cat == "RCW") return searchRCW(num);
		if (cat == "LDN") return searchLDN(num);
		if (cat == "LBN") return searchLBN(num);
		if (cat == "CR") return searchCr(num);
		if (cat == "MEL") return searchMel(num);
	}
	static QRegExp dCatNumRx("^(SH)\\s*\\d-\\s*(\\d+)$");
	if (dCatNumRx.exactMatch(uname))
	{
		QString dcat = dCatNumRx.capturedTexts().at(1);
		int dnum = dCatNumRx.capturedTexts().at(2).toInt();

		if (dcat == "SH") return searchSh2(dnum);
	}
	return NebulaP();
}

void NebulaMgr::loadNebulaSet(const QString& setName)
{
	QString ngcPath = StelFileMgr::findFile("nebulae/" + setName + "/ngc2000.dat");
	QString barnardPath = StelFileMgr::findFile("nebulae/" + setName + "/BarnardCat_tabbed.txt");
	QString sharplessPath = StelFileMgr::findFile("nebulae/" + setName + "/SharplessCat_tabbed.txt");
	QString vandenBerghPath = StelFileMgr::findFile("nebulae/" + setName + "/VandenBerghCat_tabbed.txt");
	QString rcwPath = StelFileMgr::findFile("nebulae/" + setName + "/RCWCat_tabbed.txt");
	QString ldnPath = StelFileMgr::findFile("nebulae/" + setName + "/LDNCat_tabbed.txt");
	QString lbnPath = StelFileMgr::findFile("nebulae/" + setName + "/LBNCat_tabbed.txt");
	QString ngcNamesPath = StelFileMgr::findFile("nebulae/" + setName + "/ngc2000names.dat");
	if (ngcPath.isEmpty() || ngcNamesPath.isEmpty())
	{
		qWarning() << "ERROR while loading nebula data set " << setName;
		return;
	}
	loadNGC(ngcPath);
	loadBarnard(barnardPath);
	loadSharpless(sharplessPath);
	loadVandenBergh(vandenBerghPath);
	loadRCW(rcwPath);
	loadLDN(ldnPath);
	loadLBN(lbnPath);
	loadNGCNames(ngcNamesPath);
}

// Look for a nebulae by XYZ coords
NebulaP NebulaMgr::search(const Vec3d& apos)
{
	Vec3d pos = apos;
	pos.normalize();
	NebulaP plusProche;
	float anglePlusProche=0.0f;
	foreach (const NebulaP& n, nebArray)
	{
		if (n->XYZ*pos>anglePlusProche)
		{
			anglePlusProche=n->XYZ*pos;
			plusProche=n;
		}
	}
	if (anglePlusProche>0.999f)
	{
		return plusProche;
	}
	else return NebulaP();
}


QList<StelObjectP> NebulaMgr::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;
	if (!getFlagShow())
		return result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;
	foreach (const NebulaP& n, nebArray)
	{
		equPos = n->XYZ;
		equPos.normalize();
		if (equPos*v>=cosLimFov)
		{
			result.push_back(qSharedPointerCast<StelObject>(n));
		}
	}
	return result;
}

NebulaP NebulaMgr::searchM(unsigned int M)
{
	foreach (const NebulaP& n, nebArray)
		if (n->M_nb == M)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchNGC(unsigned int NGC)
{
	if (ngcIndex.contains(NGC))
		return ngcIndex[NGC];
	return NebulaP();
}

NebulaP NebulaMgr::searchIC(unsigned int IC)
{
	foreach (const NebulaP& n, nebArray)
		if (n->IC_nb == IC) return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchC(unsigned int C)
{
	foreach (const NebulaP& n, nebArray)
		if (n->C_nb == C)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchB(unsigned int B)
{
	foreach (const NebulaP& n, nebArray)
		if (n->B_nb == B)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchSh2(unsigned int Sh2)
{
	foreach (const NebulaP& n, nebArray)
		if (n->Sh2_nb == Sh2)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchVdB(unsigned int VdB)
{
	foreach (const NebulaP& n, nebArray)
		if (n->VdB_nb == VdB)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchRCW(unsigned int RCW)
{
	foreach (const NebulaP& n, nebArray)
		if (n->RCW_nb == RCW)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchLDN(unsigned int LDN)
{
	foreach (const NebulaP& n, nebArray)
		if (n->LDN_nb == LDN)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchLBN(unsigned int LBN)
{
	foreach (const NebulaP& n, nebArray)
		if (n->LBN_nb == LBN)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchCr(unsigned int Cr)
{
	foreach (const NebulaP& n, nebArray)
		if (n->Cr_nb == Cr)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchMel(unsigned int Mel)
{
	foreach (const NebulaP& n, nebArray)
		if (n->Mel_nb == Mel)
			return n;
	return NebulaP();
}

#if 0
// read from stream
bool NebulaMgr::loadNGCOld(const QString& catNGC)
{
	QFile in(catNGC);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	int totalRecords=0;
	QString record;
	while (!in.atEnd())
	{
		in.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	in.seek(0);

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;			// how many records weree rad without problems
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		++currentLineNumber;

		// skip comments
		if (record.startsWith("//") || record.startsWith("#"))
			continue;
		++currentRecordNumber;

		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		if (!e->readNGC((char*)record.toLocal8Bit().data())) // reading error
		{
			e.clear();
		}
		else
		{
			nebArray.append(e);
			nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
			if (e->NGC_nb!=0)
				ngcIndex.insert(e->NGC_nb, e);
			++readOk;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "NGC records";
	return true;
}
#endif

bool NebulaMgr::loadNGC(const QString& catNGC)
{
	QFile in(catNGC);
	if (!in.open(QIODevice::ReadOnly))
		return false;
	QDataStream ins(&in);
	ins.setVersion(QDataStream::Qt_4_5);

	int totalRecords=0;
	while (!ins.atEnd())
	{
		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		e->readNGC(ins);

		nebArray.append(e);
		nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
		if (e->NGC_nb!=0)
			ngcIndex.insert(e->NGC_nb, e);
		++totalRecords;
	}
	in.close();
	qDebug() << "Loaded" << totalRecords << "NGC records";
	return true;
}

bool NebulaMgr::loadNGCNames(const QString& catNGCNames)
{
	qDebug() << "Loading NGC name data ...";
	QFile ngcNameFile(catNGCNames);
	if (!ngcNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "NGC name data file" << QDir::toNativeSeparators(catNGCNames) << "not found.";
		return false;
	}

	// Read the names of the NGC objects
	QString name, record, ref;
	int totalRecords=0;
	int lineNumber=0;
	int readOk=0;
	int nb;
	NebulaP e;
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	QRegExp transRx("_[(]\"(.*)\"[)]");
	while (!ngcNameFile.atEnd())
	{
		record = QString::fromUtf8(ngcNameFile.readLine());
		lineNumber++;
		if (commentRx.exactMatch(record))
			continue;

		totalRecords++;
		nb = record.mid(38,4).toInt();
		if (record[37] == 'I')
		{
			e = searchIC(nb);
			ref = "IC ";
		}
		else if (record[37] == 'B')
		{
			e = searchB(nb);
			ref = "B";
		}
		else if (record[37] == 'S')
		{
			e = searchSh2(nb);
			ref = "Sh 2-";
		}
		else if (record[37] == 'V')
		{
			e = searchVdB(nb);
			ref = "VdB ";
		}
		else if (record[37] == 'R')
		{
			e = searchRCW(nb);
			ref = "RCW ";
		}
		else if (record[37] == 'D')
		{
			e = searchLDN(nb);
			ref = "LDN ";
		}
		else if (record[37] == 'L')
		{
			e = searchLBN(nb);
			ref = "LBN ";
		}
		else
		{
			e = searchNGC(nb);
			ref = "NGC ";
		}

		// get name, trimmed of whitespace
		name = record.left(36).trimmed();

		if (e)
		{
			// If the name is not a messier number perhaps one is already
			// defined for this object
			if (name.left(2).toUpper() != "M " && name.left(2).toUpper() != "C " && name.left(2).toUpper() != "CR" && name.left(2).toUpper() != "ME")
			{
				if (transRx.exactMatch(name)) {
					e->englishName = transRx.capturedTexts().at(1).trimmed();
				}
				 else 
				{
					e->englishName = name;
				}
			}			
			else if (name.left(2).toUpper() == "M ")
			{
				// If it's a Messier number, we will call it a Messier if there is no better name
				name = name.mid(2); // remove "M "

				// read the Messier number
				QTextStream istr(&name);
				int num;
				istr >> num;
				if (istr.status()!=QTextStream::Ok)
				{
					qWarning() << "cannot read Messier number at line" << lineNumber << "of" << QDir::toNativeSeparators(catNGCNames);
					continue;
				}

				e->M_nb=(unsigned int)(num);
				e->englishName = QString("M%1").arg(num);
			}
			else if (name.left(2).toUpper() == "C ")
			{
				// If it's a Caldwell number, we will call it a Caldwell if there is no better name
				name = name.mid(2); // remove "C "

				// read the Caldwell number
				QTextStream istr(&name);
				int num;
				istr >> num;
				if (istr.status()!=QTextStream::Ok)
				{
					qWarning() << "cannot read Caldwell number at line" << lineNumber << "of" << QDir::toNativeSeparators(catNGCNames);
					continue;
				}

				e->C_nb=(unsigned int)(num);
				e->englishName = QString("C%1").arg(num);
			}			
			else if (name.left(2).toUpper() == "CR")
			{
				// If it's a Collinder number, we will call it a Messier if there is no better name
				name = name.mid(2); // remove "Cr"

				// read the Collinder number
				QTextStream istr(&name);
				int num;
				istr >> num;
				if (istr.status()!=QTextStream::Ok)
				{
					qWarning() << "cannot read Collinder number at line" << lineNumber << "of" << QDir::toNativeSeparators(catNGCNames);
					continue;
				}

				e->Cr_nb=(unsigned int)(num);
				if (e->englishName.isEmpty())
					e->englishName = QString("%1%2").arg(ref).arg(nb);
			}
			else if (name.left(2).toUpper() == "ME")
			{
				// If it's a Melotte number, we will call it a Messier if there is no better name
				name = name.mid(2); // remove "Me"

				// read the Melotte number
				QTextStream istr(&name);
				int num;
				istr >> num;
				if (istr.status()!=QTextStream::Ok)
				{
					qWarning() << "cannot read Melotte number at line" << lineNumber << "of" << QDir::toNativeSeparators(catNGCNames);
					continue;
				}

				e->Mel_nb=(unsigned int)(num);
				if (e->englishName.isEmpty())
					e->englishName = QString("%1%2").arg(ref).arg(nb);
			}
			readOk++;			
		}
		else
			qWarning() << "no position data for " << name << "at line" << lineNumber << "of" << QDir::toNativeSeparators(catNGCNames);
	}
	ngcNameFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "NGC name records successfully";

	return true;
}

bool NebulaMgr::loadBarnard(const QString& filename)
{
	QFile in(filename);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	int totalRecords=0;
	QString record;
	while (!in.atEnd())
	{
		in.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	in.seek(0);

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;			// how many records were read without problems
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		++currentLineNumber;

		// skip comments
		if (record.startsWith("//") || record.startsWith("#"))
			continue;
		++currentRecordNumber;

		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		if (!e->readBarnard(record)) // reading error
		{
			e.clear();
		}
		else
		{
			nebArray.append(e);
			nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
			++readOk;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "Barnard records";
	return true;
}

bool NebulaMgr::loadSharpless(const QString& filename)
{
	QFile in(filename);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	int totalRecords=0;
	QString record;
	while (!in.atEnd())
	{
		in.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	in.seek(0);

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;			// how many records were read without problems
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		++currentLineNumber;

		// skip comments
		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty())
			continue;
		++currentRecordNumber;

		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		if (!e->readSharpless(record)) // reading error
		{
			e.clear();
		}
		else
		{
			nebArray.append(e);
			nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
			++readOk;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "Sharpless records";
	return true;
}

bool NebulaMgr::loadVandenBergh(const QString& filename)
{
	QFile in(filename);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	int totalRecords=0;
	QString record;
	while (!in.atEnd())
	{
		in.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	in.seek(0);

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;			// how many records were read without problems
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		++currentLineNumber;

		// skip comments
		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty())
			continue;
		++currentRecordNumber;

		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		if (!e->readVandenBergh(record)) // reading error
		{
			e.clear();
		}
		else
		{
			nebArray.append(e);
			nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
			++readOk;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "Van den Bergh records";
	return true;
}

bool NebulaMgr::loadRCW(const QString& filename)
{
	QFile in(filename);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	int totalRecords=0;
	QString record;
	while (!in.atEnd())
	{
		in.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	in.seek(0);

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;			// how many records were read without problems
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		++currentLineNumber;

		// skip comments
		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty())
			continue;
		++currentRecordNumber;

		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		if (!e->readRCW(record)) // reading error
		{
			e.clear();
		}
		else
		{
			nebArray.append(e);
			nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
			++readOk;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "RCW (Rodgers+) records";
	return true;
}

bool NebulaMgr::loadLDN(const QString& filename)
{
	QFile in(filename);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	int totalRecords=0;
	QString record;
	while (!in.atEnd())
	{
		in.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	in.seek(0);

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;			// how many records were read without problems
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		++currentLineNumber;

		// skip comments
		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty())
			continue;
		++currentRecordNumber;

		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		if (!e->readLDN(record)) // reading error
		{
			e.clear();
		}
		else
		{
			nebArray.append(e);
			nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
			++readOk;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "LDN (Lynds' Catalogue of Dark Nebulae) records";
	return true;
}

bool NebulaMgr::loadLBN(const QString& filename)
{
	QFile in(filename);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	int totalRecords=0;
	QString record;
	while (!in.atEnd())
	{
		in.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	in.seek(0);

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;			// how many records were read without problems
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		++currentLineNumber;

		// skip comments
		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty())
			continue;
		++currentRecordNumber;

		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		if (!e->readLBN(record)) // reading error
		{
			e.clear();
		}
		else
		{
			nebArray.append(e);
			nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
			++readOk;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "LBN (Lynds' Catalogue of Bright Nebulae) records";
	return true;
}

void NebulaMgr::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	foreach (NebulaP n, nebArray)
		n->translateName(trans);
}


//! Return the matching Nebula object's pointer if exists or NULL
StelObjectP NebulaMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	if (objw.mid(0, 3) == "NGC")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("NGC%1").arg(n->NGC_nb) == objw || QString("NGC %1").arg(n->NGC_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by common names
	foreach (const NebulaP& n, nebArray)
	{
		QString objwcap = n->nameI18.toUpper();
		if (objwcap==objw)
			return qSharedPointerCast<StelObject>(n);
	}

	// Search by IC numbers (possible formats are "IC466" or "IC 466")
	if (objw.mid(0, 2) == "IC")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("IC%1").arg(n->IC_nb) == objw || QString("IC %1").arg(n->IC_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}


	// Search by Messier numbers (possible formats are "M31" or "M 31")
	if (objw.mid(0, 1) == "M" && objw.mid(1, 1) != "E")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("M%1").arg(n->M_nb) == objw || QString("M %1").arg(n->M_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Caldwell numbers (possible formats are "C31" or "C 31")
	if (objw.mid(0, 1) == "C" && objw.mid(1, 2) != "R")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("C%1").arg(n->C_nb) == objw || QString("C %1").arg(n->C_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Barnard numbers (possible formats are "B31" or "B 31")
	if (objw.mid(0, 1) == "B")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("B%1").arg(n->B_nb) == objw || QString("B %1").arg(n->B_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Sharpless numbers (possible formats are "Sh2-31" or "Sh 2-31")
	if (objw.mid(0, 2) == "SH")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("SH2-%1").arg(n->Sh2_nb) == objw || QString("SH 2-%1").arg(n->Sh2_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Van den Bergh numbers (possible formats are "VdB31" or "VdB 31")
	if (objw.mid(0, 3) == "VDB")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("VDB%1").arg(n->VdB_nb) == objw || QString("VDB %1").arg(n->VdB_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by RCW numbers (possible formats are "RCW31" or "RCW 31")
	if (objw.mid(0, 3) == "RCW")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("RCW%1").arg(n->RCW_nb) == objw || QString("RCW %1").arg(n->RCW_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by LDN numbers (possible formats are "LDN31" or "LDN 31")
	if (objw.mid(0, 3) == "LDN")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("LDN%1").arg(n->LDN_nb) == objw || QString("LDN %1").arg(n->LDN_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by LBN numbers (possible formats are "LBN31" or "LBN 31")
	if (objw.mid(0, 3) == "LBN")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("LBN%1").arg(n->LBN_nb) == objw || QString("LBN %1").arg(n->LBN_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Collinder numbers (possible formats are "Cr31" or "Cr 31")
	if (objw.mid(0, 2) == "CR")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("CR%1").arg(n->Cr_nb) == objw || QString("CR %1").arg(n->Cr_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Melotte numbers (possible formats are "Mel31" or "Mel 31")
	if (objw.mid(0, 3) == "MEL")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("MEL%1").arg(n->Mel_nb) == objw || QString("MEL %1").arg(n->Mel_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	return StelObjectP();
}


//! Return the matching Nebula object's pointer if exists or NULL
//! TODO split common parts of this and I18 fn above into a separate fn.
StelObjectP NebulaMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();

	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	if (objw.startsWith("NGC"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("NGC%1").arg(n->NGC_nb) == objw || QString("NGC %1").arg(n->NGC_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by common names
	foreach (const NebulaP& n, nebArray)
	{
		QString objwcap = n->englishName.toUpper();
		if (objwcap==objw)
			return qSharedPointerCast<StelObject>(n);
	}

	// Search by IC numbers (possible formats are "IC466" or "IC 466")
	if (objw.startsWith("IC"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("IC%1").arg(n->IC_nb) == objw || QString("IC %1").arg(n->IC_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Messier numbers (possible formats are "M31" or "M 31")
	if (objw.startsWith("M") && !objw.startsWith("ME"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("M%1").arg(n->M_nb) == objw || QString("M %1").arg(n->M_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Caldwell numbers (possible formats are "C31" or "C 31")
	if (objw.startsWith("C") && !objw.startsWith("CR"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("C%1").arg(n->C_nb) == objw || QString("C %1").arg(n->C_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Barnard numbers (possible formats are "B31" or "B 31")
	if (objw.startsWith("B"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("B%1").arg(n->B_nb) == objw || QString("B %1").arg(n->B_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Sharpless numbers (possible formats are "Sh2-31" or "Sh 2-31")
	if (objw.startsWith("SH"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("SH2-%1").arg(n->Sh2_nb) == objw || QString("SH 2-%1").arg(n->Sh2_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Van den Bergh numbers (possible formats are "VdB31" or "VdB 31")
	if (objw.startsWith("VDB"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("VDB%1").arg(n->VdB_nb) == objw || QString("VDB %1").arg(n->VdB_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by RCW numbers (possible formats are "RCW31" or "RCW 31")
	if (objw.startsWith("RCW"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("RCW%1").arg(n->RCW_nb) == objw || QString("RCW %1").arg(n->RCW_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by LDN numbers (possible formats are "LDN31" or "LDN 31")
	if (objw.startsWith("LDN"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("LDN%1").arg(n->LDN_nb) == objw || QString("LDN %1").arg(n->LDN_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by LBN numbers (possible formats are "LBN31" or "LBN 31")
	if (objw.startsWith("LBN"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("LBN%1").arg(n->LBN_nb) == objw || QString("LBN %1").arg(n->LBN_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Collinder numbers (possible formats are "Cr31" or "Cr 31")
	if (objw.startsWith("CR"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("CR%1").arg(n->Cr_nb) == objw || QString("CR %1").arg(n->Cr_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by Melotte numbers (possible formats are "Mel31" or "Mel 31")
	if (objw.startsWith("MEL"))
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("MEL%1").arg(n->Mel_nb) == objw || QString("MEL %1").arg(n->Mel_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	return NULL;
}


//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
QStringList NebulaMgr::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();
	// Search by Messier objects number (possible formats are "M31" or "M 31")
	if (objw.size()>=1 && objw[0]=='M' && objw[1]!='E')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->M_nb==0) continue;
			QString constw = QString("M%1").arg(n->M_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("M %1").arg(n->M_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Melotte objects number (possible formats are "Mel31" or "Mel 31")
	if (objw.size()>=1 && objw[0]=='M' && objw[1]=='E')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->Mel_nb==0) continue;
			QString constw = QString("MEL%1").arg(n->Mel_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("MEL %1").arg(n->Mel_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by IC objects number (possible formats are "IC466" or "IC 466")
	if (objw.size()>=1 && objw[0]=='I')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->IC_nb==0) continue;
			QString constw = QString("IC%1").arg(n->IC_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("IC %1").arg(n->IC_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	foreach (const NebulaP& n, nebArray)
	{
		if (n->NGC_nb==0) continue;
		QString constw = QString("NGC%1").arg(n->NGC_nb);
		QString constws = constw.mid(0, objw.size());
		if (constws==objw)
		{
			result << constws;
			continue;
		}
		constw = QString("NGC %1").arg(n->NGC_nb);
		constws = constw.mid(0, objw.size());
		if (constws==objw)
			result << constw;
	}

	// Search by Caldwell objects number (possible formats are "C31" or "C 31")
	if (objw.size()>=1 && objw[0]=='C' && objw[1]!='R')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->C_nb==0) continue;
			QString constw = QString("C%1").arg(n->C_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("C %1").arg(n->C_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Collinder objects number (possible formats are "Cr31" or "Cr 31")
	if (objw.size()>=1 && objw[0]=='C' && objw[1]=='R')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->Cr_nb==0) continue;
			QString constw = QString("CR%1").arg(n->Cr_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("CR %1").arg(n->Cr_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Barnard objects number (possible formats are "B31" or "B 31")
	if (objw.size()>=1 && objw[0]=='B')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->B_nb==0) continue;
			QString constw = QString("B%1").arg(n->B_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("B %1").arg(n->B_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Sharpless objects number (possible formats are "Sh2-31" or "Sh 2-31")
	if (objw.size()>=1 && objw[0]=='S')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->Sh2_nb==0) continue;
			QString constw = QString("SH2-%1").arg(n->Sh2_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("SH 2-%1").arg(n->Sh2_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Van den Bergh objects number (possible formats are "VdB31" or "VdB 31")
	if (objw.size()>=1 && objw[0]=='V')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->VdB_nb==0) continue;
			QString constw = QString("VDB%1").arg(n->VdB_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("VDB %1").arg(n->VdB_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by RCW objects number (possible formats are "RCW31" or "RCW 31")
	if (objw.size()>=1 && objw[0]=='R')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->RCW_nb==0) continue;
			QString constw = QString("RCW%1").arg(n->RCW_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("RCW %1").arg(n->RCW_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by LDN objects number (possible formats are "LDN31" or "LDN 31")
	if (objw.size()>=1 && objw[0]=='L' && objw[1]=='D')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->LDN_nb==0) continue;
			QString constw = QString("LDN%1").arg(n->LDN_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("LDN %1").arg(n->LDN_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by LBN objects number (possible formats are "LBN31" or "LBN 31")
	if (objw.size()>=1 && objw[0]=='L' && objw[1]=='B')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->LBN_nb==0) continue;
			QString constw = QString("LBN%1").arg(n->LBN_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("LBN %1").arg(n->LBN_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	QString dson;
	bool find;
	// Search by common names
	foreach (const NebulaP& n, nebArray)
	{
		dson = n->nameI18;
		find = false;
		if (useStartOfWords)
		{
			if (dson.mid(0, objw.size()).toUpper()==objw)
				find = true;

		}
		else
		{
			if (dson.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
			result << dson;
	}

	result.sort();
	if (maxNbItem > 0)
	{
		if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());
	}
	return result;
}

//! Find and return the list of at most maxNbItem objects auto-completing the passed object English name
QStringList NebulaMgr::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	 QString objw = objPrefix.toUpper();
	// Search by Messier objects number (possible formats are "M31" or "M 31")
	if (objw.size()>=1 && objw[0]=='M' && objw[1]!='E')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->M_nb==0) continue;
			QString constw = QString("M%1").arg(n->M_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("M %1").arg(n->M_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Melotte objects number (possible formats are "Mel31" or "Mel 31")
	if (objw.size()>=1 && objw[0]=='M' && objw[1]=='E')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->Mel_nb==0) continue;
			QString constw = QString("MEL%1").arg(n->Mel_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("MEL %1").arg(n->Mel_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by IC objects number (possible formats are "IC466" or "IC 466")
	if (objw.size()>=1 && objw[0]=='I')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->IC_nb==0) continue;
			QString constw = QString("IC%1").arg(n->IC_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("IC %1").arg(n->IC_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	foreach (const NebulaP& n, nebArray)
	{
		if (n->NGC_nb==0) continue;
		QString constw = QString("NGC%1").arg(n->NGC_nb);
		QString constws = constw.mid(0, objw.size());
		if (constws==objw)
		{
			result << constws;
			continue;
		}
		constw = QString("NGC %1").arg(n->NGC_nb);
		constws = constw.mid(0, objw.size());
		if (constws==objw)
			result << constw;
	}

	// Search by Caldwell objects number (possible formats are "C31" or "C 31")
	if (objw.size()>=1 && objw[0]=='C' && objw[1]!='R')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->C_nb==0) continue;
			QString constw = QString("C%1").arg(n->C_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("C %1").arg(n->C_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Collinder objects number (possible formats are "Cr31" or "Cr 31")
	if (objw.size()>=1 && objw[0]=='C' && objw[1]=='R')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->Cr_nb==0) continue;
			QString constw = QString("CR%1").arg(n->Cr_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("CR %1").arg(n->Cr_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Barnard objects number (possible formats are "B31" or "B 31")
	if (objw.size()>=1 && objw[0]=='B')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->B_nb==0) continue;
			QString constw = QString("B%1").arg(n->B_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("B %1").arg(n->B_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Sharpless objects number (possible formats are "Sh2-31" or "Sh 2-31")
	if (objw.size()>=1 && objw[0]=='S')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->Sh2_nb==0) continue;
			QString constw = QString("SH2-%1").arg(n->Sh2_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("SH 2-%1").arg(n->Sh2_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Van den Bergh objects number (possible formats are "VdB31" or "VdB 31")
	if (objw.size()>=1 && objw[0]=='V')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->VdB_nb==0) continue;
			QString constw = QString("VDB%1").arg(n->VdB_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("VDB %1").arg(n->VdB_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by RCW objects number (possible formats are "RCW31" or "RCW 31")
	if (objw.size()>=1 && objw[0]=='R')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->RCW_nb==0) continue;
			QString constw = QString("RCW%1").arg(n->RCW_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("RCW %1").arg(n->RCW_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by LDN objects number (possible formats are "LDN31" or "LDN 31")
	if (objw.size()>=1 && objw[0]=='L' && objw[1]=='D')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->LDN_nb==0) continue;
			QString constw = QString("LDN%1").arg(n->LDN_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("LDN %1").arg(n->LDN_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by LBN objects number (possible formats are "LBN31" or "LBN 31")
	if (objw.size()>=1 && objw[0]=='L' && objw[1]=='B')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->LBN_nb==0) continue;
			QString constw = QString("LBN%1").arg(n->LBN_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("LBN %1").arg(n->LBN_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	QString dson;
	bool find;
	// Search by common names
	foreach (const NebulaP& n, nebArray)
	{
		dson = n->englishName;
		find = false;
		if (useStartOfWords)
		{
			if (dson.mid(0, objw.size()).toUpper()==objw)
				find = true;

		}
		else
		{
			if (dson.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
			result << dson;
	}

	result.sort();
	if (maxNbItem > 0)
	{
		if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());
	}

	return result;
}

QStringList NebulaMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;
	foreach(const NebulaP& n, nebArray)
	{		
		if (!n->getEnglishName().isEmpty())
		{
			if (inEnglish)
				result << n->getEnglishName();
			else
				result << n->getNameI18n();
		}
	}
	return result;
}

QStringList NebulaMgr::listAllObjectsByType(const QString &objType, bool inEnglish) const
{
	QStringList result;
	int type = objType.toInt();
	switch (type)
	{
		case 0: // Bright galaxies?
			foreach(const NebulaP& n, nebArray)
			{
				if (n->nType==type && n->mag<=10.)
				{
					if (!n->getEnglishName().isEmpty())
					{
						if (inEnglish)
							result << n->getEnglishName();
						else
							result << n->getNameI18n();
					}
					else if (n->NGC_nb>0)
						result << QString("NGC %1").arg(n->NGC_nb);
					else
						result << QString("IC %1").arg(n->IC_nb);

				}
			}
			break;
		case 100: // Messier Catalogue?
			foreach(const NebulaP& n, nebArray)
			{
				if (n->M_nb>0)
					result << QString("M%1").arg(n->M_nb);
			}
			break;
		case 101: // Caldwell Catalogue?
			foreach(const NebulaP& n, nebArray)
			{
				if (n->C_nb>0)
					result << QString("C%1").arg(n->C_nb);
			}
			break;
		case 102: // Barnard Catalogue?
			foreach(const NebulaP& n, nebArray)
			{
				if (n->B_nb>0)
					result << QString("B %1").arg(n->B_nb);
			}
			break;
		case 103: // Sharpless Catalogue?
			foreach(const NebulaP& n, nebArray)
			{
				if (n->Sh2_nb>0)
					result << QString("Sh 2-%1").arg(n->Sh2_nb);
			}
			break;
		case 104: // Van den Bergh Catalogue
			foreach(const NebulaP& n, nebArray)
			{
				if (n->VdB_nb>0)
					result << QString("VdB %1").arg(n->VdB_nb);
			}
			break;
		case 105: // RCW Catalogue
			foreach(const NebulaP& n, nebArray)
			{
				if (n->RCW_nb>0)
					result << QString("RCW %1").arg(n->VdB_nb);
			}
			break;
		case 106: // Collinder Catalogue
			foreach(const NebulaP& n, nebArray)
			{
				if (n->Cr_nb>0)
					result << QString("Cr %1").arg(n->Cr_nb);
			}
			break;
		case 107: // Melotte Catalogue
			foreach(const NebulaP& n, nebArray)
			{
				if (n->Mel_nb>0)
					result << QString("Mel %1").arg(n->Mel_nb);
			}
			break;
		default:
			foreach(const NebulaP& n, nebArray)
			{
				if (n->nType==type)
				{
					if (!n->getEnglishName().isEmpty())
					{
						if (inEnglish)
							result << n->getEnglishName();
						else
							result << n->getNameI18n();
					}
					else if (n->NGC_nb>0)
						result << QString("NGC %1").arg(n->NGC_nb);
					else
						result << QString("IC %1").arg(n->IC_nb);

				}
			}
			break;
	}

	result.removeDuplicates();
	return result;
}

