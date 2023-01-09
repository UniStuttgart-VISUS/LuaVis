#ifndef SRC_SHARED_CONTENT_SOURCEPREFIXER_HPP_
#define SRC_SHARED_CONTENT_SOURCEPREFIXER_HPP_

#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Content/ResourceEvent.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <memory>
#include <string>
#include <vector>

namespace res
{

class SourcePrefixer : public AbstractSource
{
public:
	SourcePrefixer(std::shared_ptr<AbstractSource> source = nullptr, std::string prefix = "");
	virtual ~SourcePrefixer();

	void setSource(std::shared_ptr<AbstractSource> source);
	std::shared_ptr<AbstractSource> getSource() const;

	void setPrefix(std::string prefix);
	const std::string & getPrefix() const;

	virtual bool loadResource(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual bool loadResourceLimited(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual std::unique_ptr<Stream> openStream(const std::string & resourceName) override;
	virtual std::vector<std::string> getResourceList(std::string path, fs::ListFlags flags) const override;
	virtual bool resourceExists(const std::string & resourceName) const override;
	virtual std::string resolveToFileName(const std::string & resourceName) const override;
	virtual void pollChanges() override;

private:
	bool checkPrefix(const std::string & path) const;
	bool checkPartialPrefix(const std::string & path) const;
	std::string removePrefix(std::string filename) const;

	std::shared_ptr<AbstractSource> source;
	std::string prefix;
	Callback<ResourceEvent> callbackHandle;
};

}

#endif
