#include <Shared/Content/DirectorySource.hpp>
#include <Shared/Content/LuaSource.hpp>
#include <Shared/Content/Package.hpp>
#include <Shared/Content/PackageSource.hpp>
#include <Shared/Content/SourceAggregator.hpp>
#include <Shared/Content/SourcePrefixer.hpp>
#include <Shared/Content/ZipSource.hpp>
#include <Shared/Game/ResourceLoader.hpp>
#include <Shared/Lua/Bindings/ArrayBinding.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <memory>

namespace wos
{

ResourceLoader::ResourceLoader(res::SourceAggregator & aggregator) :
	aggregator(aggregator),
	logger("ResourceLoader")
{
}

ResourceLoader::~ResourceLoader()
{
	removeOwnedSources();
}

res::AbstractSource & ResourceLoader::getCombinedSource() const
{
	return aggregator;
}

void ResourceLoader::removeAllSources()
{
	auto baseSource = aggregator.lookUpSourceByLabel("");
	aggregator.clearSources();
	sources.clear();
	if (baseSource)
	{
		aggregator.addSource(baseSource, 0);
	}
}

void ResourceLoader::removeOwnedSources()
{
	for (const auto & source : sources)
	{
		aggregator.removeSource(*source.second);
	}
	sources.clear();
}

void ResourceLoader::setArrayContext(wosc::ArrayContext * arrayContext)
{
	this->arrayContext = arrayContext;
}

wosc::ArrayContext * ResourceLoader::getArrayContext() const
{
	return arrayContext;
}

void ResourceLoader::setExternalAssetPath(std::string externalAssetPath)
{
	this->externalAssetPath = externalAssetPath;
}

const std::string & ResourceLoader::getExternalAssetPath() const
{
	return externalAssetPath;
}

void ResourceLoader::addDirectorySource(std::string label, std::string mountPoint, int order, std::string path,
                                        bool external, bool autoReload)
{
	removeSource(label);

	auto fullPath = external ? externalAssetPath + "/" + path : path;
	auto source = std::make_shared<res::DirectorySource>(fullPath);
	source->setPollingEnabled(autoReload);
	source->setEventCoalescence(autoReloadCoalescence);
	addSource(std::make_shared<res::SourcePrefixer>(source, mountPoint), order, label);
}

void ResourceLoader::addPackageSource(std::string label, std::string mountPoint, int order, std::string path)
{
	removeSource(label);

	auto package = makeUnique<res::Package>();
	package->openFile(path);
	auto source = std::make_shared<res::PackageSource>(std::move(package));
	addSource(std::make_shared<res::SourcePrefixer>(source, mountPoint), order, label);
}

void ResourceLoader::addZipSource(std::string label, std::string mountPoint, int order, std::string path)
{
	removeSource(label);
	auto source = std::make_shared<res::ZipSource>(path);
	addSource(std::make_shared<res::SourcePrefixer>(source, mountPoint), order, label);
}

void ResourceLoader::addLuaFunctionSource(std::string label, std::string mountPoint, int order, sol::function function)
{
	removeSource(label);

	auto source = std::make_shared<res::LuaSource>(arrayContext, function);
	addSource(std::make_shared<res::SourcePrefixer>(source, mountPoint), order, label);
}

void ResourceLoader::removeSource(const std::string & label)
{
	auto foundSource = aggregator.lookUpSourceByLabel(label);
	if (foundSource)
	{
		aggregator.removeSource(*foundSource);
	}
}

bool ResourceLoader::hasSource(const std::string & label) const
{
	return aggregator.lookUpSourceByLabel(label) != nullptr;
}

void ResourceLoader::setSourceOrder(const std::string & label, int order)
{
	auto foundSource = aggregator.lookUpSourceByLabel(label);
	if (foundSource)
	{
		aggregator.setSourceOrder(*foundSource, order);
	}
}

int ResourceLoader::getSourceOrder(const std::string & label) const
{
	auto foundSource = aggregator.lookUpSourceByLabel(label);
	if (foundSource)
	{
		return aggregator.getSourceOrder(*foundSource);
	}
	else
	{
		return 0;
	}
}

void ResourceLoader::setAutoReloadCoalescence(fs::DirectoryObserver::CoalescenceSettings autoReloadCoalescence)
{
	using Settings = fs::DirectoryObserver::CoalescenceSettings;
	using Fn = std::function<void(res::AbstractSource *, const Settings &)>;

	Fn setCoalescenceImpl = [&setCoalescenceImpl](res::AbstractSource * source, const Settings & settings) -> void {
		if (auto directorySource = dynamic_cast<res::DirectorySource *>(source))
		{
			directorySource->setEventCoalescence(settings);
		}
		else if (auto aggregator = dynamic_cast<res::SourceAggregator *>(source))
		{
			for (auto subSource : aggregator->getSources())
			{
				setCoalescenceImpl(subSource.get(), settings);
			}
		}
		else if (auto prefixer = dynamic_cast<res::SourcePrefixer *>(source))
		{
			setCoalescenceImpl(prefixer->getSource().get(), settings);
		}
	};

	this->autoReloadCoalescence = autoReloadCoalescence;
	setCoalescenceImpl(&aggregator, autoReloadCoalescence);
}

fs::DirectoryObserver::CoalescenceSettings ResourceLoader::getAutoReloadCoalescence() const
{
	return autoReloadCoalescence;
}

void ResourceLoader::addSource(std::shared_ptr<res::AbstractSource> source, int order, std::string label)
{
	sources[label] = source;
	aggregator.addSource(source, order, label);
}

}
