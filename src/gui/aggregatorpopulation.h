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
#pragma once

#include "../game/creature.h"

#include <QObject>

class Game;

struct GuiSkillInfo
{
	QString sid;
	QString name;
	int level;
	float xpValue;
	bool active;
	QString group;
	QString color;
};
Q_DECLARE_METATYPE( GuiSkillInfo )


struct GuiTraitInfo
{
	QString id;
	int value;          // -50 to +50
	QString label;      // "Fearless", "Coward", or empty if not extreme
	QString description; // tooltip text
	QString category;
};
Q_DECLARE_METATYPE( GuiTraitInfo )

struct GuiBackstoryInfo
{
	QString title;
	QString description;
};
Q_DECLARE_METATYPE( GuiBackstoryInfo )

struct GuiRelationshipInfo
{
	QString name;
	unsigned int id;
	int opinion;
	QString label;  // "Rival", "Friendly", "Close Friend", etc.
};
Q_DECLARE_METATYPE( GuiRelationshipInfo )

struct GuiGnomeInfo
{
	QString name;
	unsigned int id;
	QList<GuiSkillInfo> skills;
	QString profession;
	QList<GuiTraitInfo> traits;
	GuiBackstoryInfo childhood;
	GuiBackstoryInfo adulthood;
	QList<GuiRelationshipInfo> relationships;
};
Q_DECLARE_METATYPE( GuiGnomeInfo )

struct GuiPopulationInfo
{
	QList<GuiGnomeInfo> gnomes;
};
Q_DECLARE_METATYPE( GuiPopulationInfo )

struct GuiGnomeScheduleInfo
{
	QString name;
	unsigned int id;
	QList<ScheduleActivity> schedule;
};
Q_DECLARE_METATYPE( GuiGnomeScheduleInfo )

struct GuiScheduleInfo
{
	QList<GuiGnomeScheduleInfo> schedules;
};
Q_DECLARE_METATYPE( GuiScheduleInfo )



class AggregatorPopulation : public QObject
{
	Q_OBJECT

public:
	AggregatorPopulation( QObject* parent = nullptr );

	void init( Game* game );

private:
	QPointer<Game> g;

	GuiPopulationInfo m_populationInfo;

	GuiScheduleInfo m_scheduleInfo;

	QList<GuiSkillInfo> m_skillIds;
	
	QList<GuiSkillInfo> m_profSkills;

	QString m_sortMode = "Name";
	bool m_revertSort = false;

public slots:
	void onRequestPopulationUpdate();
	void onUpdateSingleGnome( unsigned int gnomeID );
	void onSetSkillActive( unsigned int gnomeID, QString skillID, bool value );
	void onSetAllSkills( unsigned int gnomeID, bool value );
	void onSetAllGnomes( QString skillID, bool value );
	void onSetProfession( unsigned int gnomeID, QString profession );
	void onSortGnomes( QString mode );

	void onRequestSchedules();
	void onSetSchedule( unsigned int gnomeID, int hour, ScheduleActivity activity );
	void onSetAllHours( unsigned int gnomeID, ScheduleActivity activity );
	void onSetHourForAll( int hour, ScheduleActivity activity );

	void onRequestProfessions();
	void onRequestSkills( QString profession );

	void onUpdateProfession( QString name, QString newName, QStringList skills );
	void onDeleteProfession( QString name );
	void onNewProfession();
signals:
	void signalPopulationUpdate( const GuiPopulationInfo& info );
	void signalProfessionList( const QStringList& professions );
	void signalProfessionSkills( QString profession, const QList<GuiSkillInfo>& skills );

	void signalUpdateSingleGnome( const GuiGnomeInfo& gnome );

	void signalScheduleUpdate( const GuiScheduleInfo& info );
	void signalScheduleUpdateSingleGnome( const GuiGnomeScheduleInfo& info );
	void signalSelectEditProfession( const QString name );
};