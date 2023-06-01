/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** ConfigFile.cpp
** 
**
** -------------------------------------------------------------------------*/

#include "ConfigFile.h"
#include <algorithm>
#include <locale>
#include <fstream>


const std::string kEmptyString;
const std::string kSectionStart = "[";
const std::string kSectionEnd = "]";
const std::string kEqual = "=";
const auto kNonSpaceFn = [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); };


static std::string& trim(std::string &str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), kNonSpaceFn));
    str.erase(std::find_if(str.rbegin(), str.rend(), kNonSpaceFn).base(), str.end());

    return str;
}


static std::string trim(const std::string &str)
{
    std::string strCopy = str;
    return trim(strCopy);
}


ConfigFile::ConfigFile()
    : hasChanges(false)
{}


ConfigFile::ConfigFile(const std::string &path)
    : path(path)
    , hasChanges(false)
{
    reload();
}


ConfigFile::~ConfigFile()
{
    save();
}


std::vector<std::string> ConfigFile::listSections() const
{
    std::vector<std::string> sections;
    sections.reserve(config.size());

    for (const auto &it : config)
    {
        sections.push_back(it.first);
    }

    return sections;
}


std::vector<std::string> ConfigFile::listKeys(const std::string &section) const
{
    std::vector<std::string> keys;

    const auto sectionIt = config.find(section);
    if (sectionIt != config.end())
    {
        keys.reserve(sectionIt->second.size());
        for (const auto &it : sectionIt->second)
        {
            keys.push_back(it.first);
        }
    }

    return keys;
}


const std::string &ConfigFile::getValue(const std::string &section, const std::string &key) const
{
    const std::string *valuePtr = &kEmptyString;

    const auto sectionIt = config.find(trim(section));

    if (sectionIt != config.end())
    {
        const auto keyIt = sectionIt->second.find(trim(key));
        if (keyIt != sectionIt->second.end())
        {
            valuePtr = &keyIt->second;
        }
    }

    return *valuePtr;
}


bool ConfigFile::hasValue(const std::string &section, const std::string &key) const
{
    return !getValue(section, key).empty();
}


void ConfigFile::setValue(const std::string &section, const std::string &key, const std::string &value)
{
    auto &valueRef = trim(config[trim(section)][trim(key)]);
    const auto &newValue = trim(value);

    hasChanges = hasChanges || valueRef != newValue;

    valueRef = newValue;
}


void ConfigFile::reload()
{
    reload(path);
}


void ConfigFile::reload(const std::string &newPath)
{
    path = newPath;

    std::ifstream file(path, std::ios_base::in);

    if (file.is_open())
    {
        config.clear();

        std::string section;
        std::string key;
        std::string value;

        for (std::string line; std::getline(file, line);)
        {
            if (parseLine(line, &section, &key, &value))
            {
                config[section][key] = value;
            }
        }

        file.close();
    }
}


bool ConfigFile::parseLine(const std::string &line, std::string *inOutsection, std::string *outKey, std::string *outValue) const
{
    bool isParsed = false;

    std::string trimmedLine = trim(line);

    if (trimmedLine.size() > 2)
    {
        if (trimmedLine.size() > kSectionStart.size() + kSectionEnd.size() &&
            !trimmedLine.compare(0, kSectionStart.size(), kSectionStart) &&
            !trimmedLine.compare(trimmedLine.size() - kSectionEnd.size(), kSectionEnd.size(), kSectionEnd))
        {
            *inOutsection = trim(trimmedLine.substr(kSectionStart.size(), trimmedLine.size() - kSectionStart.size() - kSectionEnd.size()));
        }
        else
        {
            const size_t delimPos = trimmedLine.find(kEqual);
            if (delimPos != std::string::npos)
            {
                *outKey = trim(trimmedLine.substr(0, delimPos));
                *outValue = trim(trimmedLine.substr(delimPos + 1));

                isParsed = !outKey->empty();
            }
        }
    }

    return isParsed;
}


void ConfigFile::save()
{
    if (!config.empty() && hasChanges)
    {
        std::ofstream file(path, std::ios_base::trunc | std::ios_base::out);

        if (file.is_open())
        {
            for (const auto &section : config)
            {
                if (!section.first.empty())
                {
                    file << kSectionStart << section.first << kSectionEnd << std::endl;
                }

                for (const auto &key : section.second)
                {
                    file << key.first << kEqual << key.second << std::endl;
                }
            }

            file.close();

            hasChanges = false;
        }
    }
}


void ReadOnlyConfigSection::init(const std::shared_ptr<ConfigFile> &configFile, const std::string &section)
{
    this->configFile = configFile;
    this->section = section;
}


const std::string& ReadOnlyConfigSection::getValue(const std::string &key) const
{
    return configFile
        ? configFile->getValue(section, key)
        : kEmptyString;
}





