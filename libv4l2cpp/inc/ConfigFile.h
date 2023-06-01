/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** ConfigFile.h
** 
**
** -------------------------------------------------------------------------*/

#ifndef CONFIG_FILE
#define CONFIG_FILE


#include <string>
#include <map>
#include "memory"
#include <vector>


class ConfigFile
{
public:
    ConfigFile();
    explicit ConfigFile(const std::wstring &path);
    virtual ~ConfigFile();

    std::vector<std::string> listSections() const;
    std::vector<std::string> listKeys(const std::string &section) const;
    const std::string &getValue(const std::string &section, const std::string &key) const;
    void setValue(const std::string &section, const std::string &key, const std::string &value);

    template<typename Section, typename Val>
    Val getValue(const Section &section, const std::string &key, const Val &defVal) const;

    template<typename Val>
    Val getValue(const std::string &section, const std::string &key, const Val &defVal) const;

    template<typename Section, typename Val>
    void setValue(const Section &section, const std::string &key, const Val &value);

    template<typename Val>
    void setValue(const std::string &section, const std::string &key, const Val &value);

    void reload(const std::wstring &newPath);
    void reload();
    void save();

private:
    bool parseLine(const std::string &line, std::string *inOutsection, std::string *outKey, std::string *outValue) const;

private:
    std::wstring path;
    std::map<std::string, std::map<std::string, std::string>> config;
    bool hasChanges;

};


class ReadOnlyConfigSection
{
public:
    ReadOnlyConfigSection(const std::shared_ptr<ConfigFile> &configFile, const std::string &section);

    template<typename Section>
    ReadOnlyConfigSection(const std::shared_ptr<ConfigFile> &configFile, const Section &section);

    const std::string& getValue(const std::string &key) const;

    template<typename Val>
    Val getValue(const std::string &key, const Val &defVal) const;

private:
    std::shared_ptr<ConfigFile> configFile;
    std::string section;
};


template<typename Section, typename Val>
Val ConfigFile::getValue(const Section &section, const std::string &key, const Val &defVal) const
{
    return getValue(std::to_string(section), key, defVal);
}


template<typename Val>
Val ConfigFile::getValue(const std::string &section, const std::string &key, const Val &defVal) const
{
    const auto strVal = getValue(section, key);

    if (!strVal.empty())
    {
        try
        {
            return static_cast<Val>(std::stoll(strVal));
        }
        catch (...) {}
    }

    return defVal;
}


template<typename Section, typename Val>
void ConfigFile::setValue(const Section &section, const std::string &key, const Val &value)
{
    setValue(std::to_string(section), key, value);
}


template<typename Val>
void ConfigFile::setValue(const std::string &section, const std::string &key, const Val &value)
{
    setValue(section, key, std::to_string(value));
}


template<typename Section>
ReadOnlyConfigSection::ReadOnlyConfigSection(const std::shared_ptr<ConfigFile> &configFile, const Section &section)
    :ReadOnlyConfigSection(configFile, std::to_string(section))
{}


template<typename Val>
Val ReadOnlyConfigSection::getValue(const std::string &key, const Val &defVal) const
{
    return configFile
        ? configFile->getValue(section, key, defVal)
        : defVal;
}


#endif


