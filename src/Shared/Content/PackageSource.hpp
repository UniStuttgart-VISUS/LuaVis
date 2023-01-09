#ifndef SRC_SHARED_CONTENT_PACKAGESOURCE_HPP_
#define SRC_SHARED_CONTENT_PACKAGESOURCE_HPP_

#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <memory>
#include <string>
#include <vector>

namespace res
{

class Package;

class PackageSource : public AbstractSource
{
public:
	static const std::string PACKAGE_HASH;

	PackageSource();
	PackageSource(std::unique_ptr<Package> package);
	virtual ~PackageSource();

	void setPackage(std::unique_ptr<Package> package);
	Package * getPackage() const;

	virtual bool loadResource(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual bool resourceExists(const std::string & resourceName) const override;
	virtual std::vector<std::string> getResourceList(std::string prefix, fs::ListFlags flags) const override;

private:
	Logger logger;

	std::unique_ptr<Package> package;

	mutable std::vector<std::string> fileListCache;
	mutable std::vector<std::string> directoryListCache;
};

}

#endif
