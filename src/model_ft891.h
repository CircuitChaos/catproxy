#include "model.h"

class ModelFT891 : public Model {
public:
	virtual ~ModelFT891() {}

	virtual unsigned getMaxCookedSize() const;
	virtual unsigned getMeterCount() const;
	virtual void reset();
	virtual std::string start();
	virtual std::optional<std::string> handleResponse(const std::string &rsp);
	virtual Meters getMeters();

	static std::string getName();
	static ModelMeterList getMeterList();

private:
	enum State {
		STATE_OFF, /* Waiting for start() */
		STATE_IDD, /* Reading Idd, it's also to determine mode (RX or TX) */
		STATE_SIG, /* Idd was 0, so only reading signal */
		STATE_CMP,
		STATE_ALC,
		STATE_PWR,
		STATE_SWR,
		STATE_DONE, /* All done, meters can be read now */
	};

	Meters meters;
	State state{STATE_OFF};

	std::string createCommand(unsigned meter);
	bool parseResponse(const std::string &rsp, unsigned &meter, uint8_t &raw);
	void createDefaultMeters();
	void addMeter(unsigned meter, uint8_t raw);

	static std::string cookSig(uint8_t raw);
	static std::string cookCmp(uint8_t raw);
	static std::string cookAlc(uint8_t raw);
	static std::string cookPwr(uint8_t raw);
	static std::string cookSwr(uint8_t raw);
	static std::string cookIdd(uint8_t raw);
};
