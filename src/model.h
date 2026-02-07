#pragma once

#include <string>
#include <set>
#include <map>
#include <optional>
#include "meters.h"

class Model {
public:
	virtual ~Model() {}

	/* Returns maximum number of characters in cooked meter texts. */
	virtual unsigned getMaxCookedSize() const = 0;

	/* Returns number of meters the model supports */
	virtual unsigned getMeterCount() const = 0;

	/* Resets the model, should be called after CAT timeout to have a fresh
	 * start once radio is back online. */
	virtual void reset() = 0;

	/* Starts polling meters for this model. Returns CAT command to send,
	 * including trailing ';'. */
	virtual std::string start() = 0;

	/* Handles CAT response, returns nullopt if it was not processed (it's meant
	 * to be sent to the radio), or a command to send to the radio (to poll next
	 * meter). If command is an empty string, then polling is done and you can
	 * call getMeters().
	 *
	 * CAT response has to be with trailing ';', CAT command is also with it.
	 */
	virtual std::optional<std::string> handleResponse(const std::string &rsp) = 0;

	/* Returns meters. Should be called after handleResponse() returns empty string.
	 * Note that all meters are returned, even those that are not meaningful (for
	 * example TX-related meters in RX mode).
	 *
	 * Once this function is called, subsequent calls return an empty vector.
	 *
	 * Empty vector means an error.
	 */
	virtual Meters getMeters() = 0;
};

typedef std::set<std::string> ModelMeterList;
typedef std::map<std::string, ModelMeterList> ModelList;
ModelList getModelList();
Model *createModel(const std::string &name);
