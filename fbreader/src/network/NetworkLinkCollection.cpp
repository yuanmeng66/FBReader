/*
 * Copyright (C) 2008-2010 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <cctype>
#include <algorithm>

#include <ZLFile.h>
#include <ZLDir.h>
#include <ZLStringUtil.h>
#include <ZLUnicodeUtil.h>
#include <ZLResource.h>
#include <ZLNetworkManager.h>
#include <ZLNetworkUtil.h>

#include "NetworkLinkCollection.h"

#include "../options/FBCategoryKey.h"

#include "../database/booksdb/BooksDB.h"

#include "NetworkOperationData.h"
#include "NetworkBookCollection.h"
#include "NetworkAuthenticationManager.h"

#include "litres/LitResLink.h"
#include "opdsLink/OPDSLinkReader.h"
#include "opdsLink/URLRewritingRule.h"

NetworkLinkCollection *NetworkLinkCollection::ourInstance = 0;

NetworkLinkCollection &NetworkLinkCollection::Instance() {
	if (ourInstance == 0) {
		ourInstance = new NetworkLinkCollection();
	}
	return *ourInstance;
}

class NetworkLinkCollection::Comparator {

public:
	bool operator() (
		const shared_ptr<NetworkLink> &first,
		const shared_ptr<NetworkLink> &second
	) const;

private:
	std::string removeLeadingNonAscii(const std::string &title) const;
};

std::string NetworkLinkCollection::Comparator::removeLeadingNonAscii(const std::string &title) const {
	std::string str = title;
	std::string::iterator it = str.begin();
	for (; it != str.end(); ++it) {
		if (((*it & 0x80) == 0) && isalnum(*it)) {
			break;
		}
	}
	if (it != str.end()) {
		str.erase(str.begin(), it);
	}
	return str;
}

bool NetworkLinkCollection::Comparator::operator() (
	const shared_ptr<NetworkLink> &first,
	const shared_ptr<NetworkLink> &second
) const {
	return
		removeLeadingNonAscii(first->Title) <
		removeLeadingNonAscii(second->Title);
}

NetworkLinkCollection::NetworkLinkCollection() :
	DirectoryOption(ZLCategoryKey::NETWORK, "Options", "DownloadDirectory", "") {
	myLinks.push_back(new LitResLink());

	shared_ptr<ZLDir> dir = ZLFile(NetworkLink::NetworkDataDirectory()).directory();
	if (!dir.isNull()) {
		std::vector<std::string> names;
		dir->collectFiles(names, false);
		for (std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it) {
			OPDSLinkReader reader;
			reader.readDocument(dir->itemPath(*it));
			shared_ptr<NetworkLink> link = reader.link();
			if (!link.isNull()) {
				myLinks.push_back(link);
			}
		}
	}

	std::sort(myLinks.begin(), myLinks.end(), Comparator());
}

NetworkLinkCollection::~NetworkLinkCollection() {
}

static std::string normalize(const std::string &url) {
	static const std::string PREFIX0 = "http://feedbooks.com/";
	static const std::string PREFIX1 = "http://www.feedbooks.com/";
	static const std::string STANZA_PREFIX = "http://feedbooks.com/book/stanza/";

	std::string nURL = url;
	if (ZLStringUtil::stringStartsWith(nURL, PREFIX1)) {
		nURL = PREFIX0 + nURL.substr(PREFIX1.length());
	}
	if (ZLStringUtil::stringStartsWith(nURL, STANZA_PREFIX)) {
		nURL = PREFIX0 + "book/" + nURL.substr(STANZA_PREFIX.length()) + ".epub";
	}
	return nURL;
}

std::string NetworkLinkCollection::makeBookFileName(const std::string &url, NetworkBookItem::URLType format) {
	myErrorMessage.clear();
	return makeBookFileName(url, format, false);
}

std::string NetworkLinkCollection::makeBookFileName(const std::string &url, NetworkBookItem::URLType format, bool createDirectories) {
	const ZLResource &errorResource = ZLResource::resource("dialog")["networkError"];
	size_t index = url.find("://");
	std::string suburl = url.substr(index + 3);
	if (ZLStringUtil::stringStartsWith(suburl, "www.")) {
		suburl.erase(0, 4);
	}
	shared_ptr<ZLDir> dir;
	std::string fileName = DirectoryOption.value();
	index = 0;
	while (true) {
		dir = ZLFile(fileName).directory(createDirectories);
		if (dir.isNull()) {
			if (createDirectories) {
				myErrorMessage = ZLStringUtil::printf(errorResource["couldntCreateDirectoryMessage"].value(), fileName);
			}
			return "";
		}
		size_t index2 = suburl.find('/', index);
		if (index2 == std::string::npos) {
			break;
		}
		if (index2 + 1 < suburl.size() && suburl[index2 + 1] == '?') {
			suburl.erase(index2, 1);
			break;
		}
		fileName = dir->itemPath(suburl.substr(index, index2 - index));
		index = index2 + 1;
	}

	size_t index2 = suburl.find('?', index);
	std::string name;
	std::string params;
	if (index2 != std::string::npos) {
		name = suburl.substr(index, index2 - index);
		params = suburl.substr(index2 + 1);
	} else {
		name = suburl.substr(index);
	}

	std::string ext;
	std::string suffix;
	switch (format) {
		case NetworkBookItem::BOOK_EPUB:
			ext = ".epub";
			break;
		case NetworkBookItem::BOOK_MOBIPOCKET:
			ext = ".mobi";
			break;
		case NetworkBookItem::BOOK_FB2_ZIP:
			ext = ".fb2.zip";
			break;
		case NetworkBookItem::BOOK_DEMO_FB2_ZIP:
			suffix = ".trial";
			ext = ".fb2.zip";
			break;
		case NetworkBookItem::NONE:
		case NetworkBookItem::LINK_HTTP:
			break;
	}

	if (ext.empty()) {
		size_t j = name.rfind('.');
		if (j != std::string::npos) {
			ext = name.substr(j);
			name.erase(j);
		}
	} else if (ZLStringUtil::stringEndsWith(name, ext)) {
		name.erase(name.size() - ext.size());
	}

	index2 = 0;
	while (params.size() > index2) {
		size_t j = params.find('&', index2);

		std::string param = params.substr(index2, j - index2);
		if (!ZLStringUtil::stringStartsWith(param, "username=")
			&& !ZLStringUtil::stringStartsWith(param, "password=")) {

			name.append("_").append(param);
			const size_t backIndex = name.size() - 1;
			if (name[backIndex] == '=') {
				name.erase(backIndex);
			}
		}

		if (j == std::string::npos) {
			break;
		}
		index2 = j + 1;
	}
	name.append(suffix).append(ext);
	return dir->itemPath(name);
}

std::string NetworkLinkCollection::bookFileName(const std::string &networkBookId) const {
	return BooksDB::Instance().getNetFile(::normalize(networkBookId));
}

bool NetworkLinkCollection::downloadBook(const std::string &url, const std::string &networkBookId, NetworkBookItem::URLType format, std::string &fileName, const ZLNetworkSSLCertificate &sslCertificate, shared_ptr<ZLExecutionData::Listener> listener) {
	const std::string nURL = ::normalize(url);
	const std::string nNetworkBookId = ::normalize(networkBookId);
	const ZLResource &errorResource = ZLResource::resource("dialog")["networkError"];
	myErrorMessage.clear();

	if (nURL.empty() || nNetworkBookId.empty()) {
		myErrorMessage = errorResource["unknownErrorMessage"].value();
		return false;
	}
	std::string storedFileName = BooksDB::Instance().getNetFile(nNetworkBookId);
	if (!storedFileName.empty() && ZLFile(storedFileName).exists()) {
		fileName = storedFileName;
		return true;
	}
	fileName = makeBookFileName(nNetworkBookId, format, true);
	if (fileName.empty()) {
		if (myErrorMessage.empty()) {
			myErrorMessage = errorResource["unknownErrorMessage"].value();
		}
		return false;
	}
	if (ZLFile(fileName).exists()) {
		ZLFile(fileName).remove();
	}
	BooksDB::Instance().setNetFile(nNetworkBookId, fileName);
	myErrorMessage = ZLNetworkManager::Instance().downloadFile(nURL, sslCertificate, fileName, listener);
	return myErrorMessage.empty();
}

shared_ptr<NetworkBookCollection> NetworkLinkCollection::simpleSearch(const std::string &pattern) {
	ZLExecutionData::Vector dataList;
	std::vector<shared_ptr<NetworkOperationData> > searchDatas;
	shared_ptr<NetworkBookCollection> result;

	myErrorMessage.clear();

	for (LinkVector::const_iterator it = myLinks.begin(); it != myLinks.end(); ++it) {
		NetworkLink &link = **it;
		shared_ptr<NetworkOperationData> searchData = new NetworkOperationData(link);
		searchDatas.push_back(searchData);
		if (link.OnOption.value()) {
			shared_ptr<ZLExecutionData> data = link.simpleSearchData(*searchData, pattern);
			if (!data.isNull()) {
				dataList.push_back(data);
			}
		}
	}

	while (myErrorMessage.empty() && !dataList.empty()) {
		myErrorMessage = ZLNetworkManager::Instance().perform(dataList);

		for (std::vector<shared_ptr<NetworkOperationData> >::const_iterator jt = searchDatas.begin(); jt != searchDatas.end(); ++jt) {
			NetworkOperationData &searchData = **jt;
			if (!searchData.Items.empty() && result.isNull()) {
				result = new NetworkBookCollection();
			}
			for (NetworkLibraryItem::List::const_iterator kt = searchData.Items.begin(); kt != searchData.Items.end(); ++kt) {
				result->addBook(*kt);
			}
		}

		dataList.clear();

		for (std::vector<shared_ptr<NetworkOperationData> >::const_iterator jt = searchDatas.begin(); jt != searchDatas.end(); ++jt) {
			NetworkOperationData &searchData = **jt;
			const NetworkLink &link = searchData.Link;
			if (link.OnOption.value()) {
				shared_ptr<ZLExecutionData> data = link.resume(searchData);
				if (!data.isNull()) {
					dataList.push_back(data);
				}
			}
		}
	}

	return result;
}

shared_ptr<NetworkBookCollection> NetworkLinkCollection::advancedSearch(const std::string &titleAndSeries, const std::string &author, const std::string &tag, const std::string &annotation) {
	ZLExecutionData::Vector dataList;
	std::vector<shared_ptr<NetworkOperationData> > searchDatas;
	shared_ptr<NetworkBookCollection> result;

	myErrorMessage.clear();

	for (LinkVector::const_iterator it = myLinks.begin(); it != myLinks.end(); ++it) {
		NetworkLink &link = **it;
		shared_ptr<NetworkOperationData> searchData = new NetworkOperationData(link);
		searchDatas.push_back(searchData);
		if (link.OnOption.value()) {
			shared_ptr<ZLExecutionData> data = link.advancedSearchData(*searchData, titleAndSeries, author, tag, annotation);
			if (!data.isNull()) {
				dataList.push_back(data);
			}
		}
	}

	while (myErrorMessage.empty() && !dataList.empty()) {
		myErrorMessage = ZLNetworkManager::Instance().perform(dataList);

		for (std::vector<shared_ptr<NetworkOperationData> >::const_iterator jt = searchDatas.begin(); jt != searchDatas.end(); ++jt) {
			NetworkOperationData &searchData = **jt;
			if (!searchData.Items.empty() && result.isNull()) {
				result = new NetworkBookCollection();
			}
			for (NetworkLibraryItem::List::const_iterator kt = searchData.Items.begin(); kt != searchData.Items.end(); ++kt) {
				result->addBook(*kt);
			}
		}

		dataList.clear();

		for (std::vector<shared_ptr<NetworkOperationData> >::const_iterator jt = searchDatas.begin(); jt != searchDatas.end(); ++jt) {
			NetworkOperationData &searchData = **jt;
			const NetworkLink &link = searchData.Link;
			if (link.OnOption.value()) {
				shared_ptr<ZLExecutionData> data = link.resume(searchData);
				if (!data.isNull()) {
					dataList.push_back(data);
				}
			}
		}
	}

	return result;
}

size_t NetworkLinkCollection::size() const {
	return myLinks.size();
}

NetworkLink &NetworkLinkCollection::link(size_t index) const {
	return *myLinks[index];
}

size_t NetworkLinkCollection::numberOfEnabledLinks() const {
	size_t count = 0;
	for (LinkVector::const_iterator it = myLinks.begin(); it != myLinks.end(); ++it) {
		if ((*it)->OnOption.value()) {
			++count;
		}
	}
	return count;
}

void NetworkLinkCollection::rewriteUrl(std::string &url, bool externalUrl) const {
	const std::string host =
		ZLUnicodeUtil::toLower(ZLNetworkUtil::hostFromUrl(url));
	for (LinkVector::const_iterator it = myLinks.begin(); it != myLinks.end(); ++it) {
		if (host.find((*it)->SiteName) != std::string::npos) {
			(*it)->rewriteUrl(url, externalUrl);
		}
	}
}
