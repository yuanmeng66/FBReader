/*
 * Copyright (C) 2009-2010 Geometer Plus <contact@geometerplus.com>
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

#ifndef __LITRESAUTHENTICATIONMANAGER_H__
#define __LITRESAUTHENTICATIONMANAGER_H__

#include <set>

#include <ZLNetworkSSLCertificate.h>

#include "../NetworkAuthenticationManager.h"
#include "../NetworkLibraryItems.h"

class LitResLink;

class LitResAuthenticationManager : public NetworkAuthenticationManager {

public:
	LitResAuthenticationManager(const LitResLink &link);

public:
	AuthenticationStatus isAuthorised(bool useNetwork = true);
	std::string authorise(const std::string &pwd);
	void logOut();
	bool skipIPSupported();

	std::string networkBookId(const NetworkBookItem &book); 
	NetworkBookItem::URLType downloadLinkType(const NetworkBookItem &book);

	const std::string &currentUserName();
	bool needsInitialization();
	std::string initialize();
	bool needPurchase(const NetworkBookItem &book);
	std::string purchaseBook(NetworkBookItem &book);
	std::string downloadLink(const NetworkBookItem &book);

	std::string refillAccountLink();
	std::string currentAccount();

	std::string reloadPurchasedBooks();
	void collectPurchasedBooks(NetworkLibraryItem::List &list);

private:
	shared_ptr<ZLExecutionData> loadPurchasedBooks(std::set<std::string> &purchasedBooksIds, NetworkLibraryItem::List &purchasedBooksList);
	void loadPurchasedBooksOnError(std::set<std::string> &purchasedBooksIds, NetworkLibraryItem::List &purchasedBooksList);
	void loadPurchasedBooksOnSuccess(std::set<std::string> &purchasedBooksIds, NetworkLibraryItem::List &purchasedBooksList);

	shared_ptr<ZLExecutionData> loadAccount(std::string &dummy1);
	void loadAccountOnError();
	void loadAccountOnSuccess();

	const ZLNetworkSSLCertificate &certificate();

public: // new User Registration
	bool registrationSupported();
	std::string registerUser(const std::string &login, const std::string &password, const std::string &email);

public: // Password Recovery
	bool passwordRecoverySupported();
	std::string recoverPassword(const std::string &email);

private:
	const LitResLink &myLink;

	bool mySidChecked;

	ZLStringOption mySidUserNameOption;
	ZLStringOption mySidOption;

	std::string myInitializedDataSid;
	std::set<std::string> myPurchasedBooksIds;
	NetworkLibraryItem::List myPurchasedBooksList;
	std::string myAccount;

	ZLNetworkSSLCertificate myCertificate;
};

#endif /* __LITRESAUTHENTICATIONMANAGER_H__ */
