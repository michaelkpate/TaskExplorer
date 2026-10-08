#pragma once

#include "../../taskcore_global.h"

#include "../../../MiscHelpers/Common/Common.h"
#include "../ProcessInfo.h"

class TASKCORE_EXPORT CGpuMonitor : public QObject
{
	Q_OBJECT

	TRACK_OBJECT(CGpuMonitor)
public:
	CGpuMonitor(QObject *parent = nullptr);
    virtual ~CGpuMonitor();

	virtual bool		Init() = 0;

	virtual bool		UpdateAdapters() = 0;

	virtual bool		UpdateGpuStats() = 0;

	struct SGpuMemory
	{
		SGpuMemory()
		{
			DedicatedLimit = 0;
			DedicatedUsage = 0;
			SharedLimit = 0;
			SharedUsage = 0;
		}

		quint64 DedicatedLimit;
		quint64 DedicatedUsage;
		quint64	SharedLimit;
		quint64	SharedUsage;
	};

	//
	// What a GPU engine is for. A driver that reports no metadata leaves the
	// type unknown, and such a node is named by its index alone.
	//
	enum EEngineType
	{
		eEngineOther			= 0,	// DXGK_ENGINE_TYPE_OTHER: the driver named it itself
		eEngine3D				= 1,
		eEngineVideoDecode		= 2,
		eEngineVideoEncode		= 3,
		eEngineVideoProcessing	= 4,
		eEngineSceneAssembly	= 5,
		eEngineCopy				= 6,
		eEngineOverlay			= 7,
		eEngineCrypto			= 8,

		eEngineUnknown			= -1,	// no metadata to go on
	};

	struct SGpuNode
	{
		SGpuNode(quint32 index = 0, int engineType = eEngineUnknown, const QString& friendlyName = QString())
		{
			Index = index;
			EngineType = engineType;
			FriendlyName = friendlyName;
			TimeUsage = 0;
		}
		quint32 Index;
		int EngineType;

		//
		// The name the driver gave an engine it did not otherwise classify.
		// Only meaningful for eEngineOther.
		//
		QString FriendlyName;

		float TimeUsage;
	};

	struct SGpuInfo
	{
		SGpuInfo()
		{
			InstalledMemory = 0;
			VendorID = 0;
			DeviceID = 0;

			TimeUsage = 0;
		}

		QString DeviceInterface;
		QString Description;
		QString DriverDate;
		QString DriverVersion;
		QString LocationInfo;
		quint64 InstalledMemory;
		//QString WDDMVersion;
		quint32 VendorID;
		quint32 DeviceID;

		float TimeUsage;

		SGpuMemory Memory;
		bool UsageAvailable = false;
		QList<SGpuNode> Nodes;
	};

	virtual QMap<QString, SGpuInfo>	GetAllGpuList() = 0;
	virtual SGpuMemory				GetGpuMemory() = 0;

protected:

	mutable QReadWriteLock		m_StatsMutex;
};
