// MIT License

// Copyright (c) 2017 Zhuhao Wang

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "nekit/utils/country_iso_code.h"

#include <unordered_map>

namespace nekit {
namespace utils {

static const std::unordered_map<std::string, CountryIsoCode> country_code_map =
    {{"AF", CountryIsoCode::AF}, {"AX", CountryIsoCode::AX},
     {"AL", CountryIsoCode::AL}, {"DZ", CountryIsoCode::DZ},
     {"AS", CountryIsoCode::AS}, {"AD", CountryIsoCode::AD},
     {"AO", CountryIsoCode::AO}, {"AI", CountryIsoCode::AI},
     {"AQ", CountryIsoCode::AQ}, {"AG", CountryIsoCode::AG},
     {"AR", CountryIsoCode::AR}, {"AM", CountryIsoCode::AM},
     {"AW", CountryIsoCode::AW}, {"AU", CountryIsoCode::AU},
     {"AT", CountryIsoCode::AT}, {"AZ", CountryIsoCode::AZ},
     {"BS", CountryIsoCode::BS}, {"BH", CountryIsoCode::BH},
     {"BD", CountryIsoCode::BD}, {"BB", CountryIsoCode::BB},
     {"BY", CountryIsoCode::BY}, {"BE", CountryIsoCode::BE},
     {"BZ", CountryIsoCode::BZ}, {"BJ", CountryIsoCode::BJ},
     {"BM", CountryIsoCode::BM}, {"BT", CountryIsoCode::BT},
     {"BO", CountryIsoCode::BO}, {"BQ", CountryIsoCode::BQ},
     {"BA", CountryIsoCode::BA}, {"BW", CountryIsoCode::BW},
     {"BV", CountryIsoCode::BV}, {"BR", CountryIsoCode::BR},
     {"IO", CountryIsoCode::IO}, {"BN", CountryIsoCode::BN},
     {"BG", CountryIsoCode::BG}, {"BF", CountryIsoCode::BF},
     {"BI", CountryIsoCode::BI}, {"CV", CountryIsoCode::CV},
     {"KH", CountryIsoCode::KH}, {"CM", CountryIsoCode::CM},
     {"CA", CountryIsoCode::CA}, {"KY", CountryIsoCode::KY},
     {"CF", CountryIsoCode::CF}, {"TD", CountryIsoCode::TD},
     {"CL", CountryIsoCode::CL}, {"CN", CountryIsoCode::CN},
     {"CX", CountryIsoCode::CX}, {"CC", CountryIsoCode::CC},
     {"CO", CountryIsoCode::CO}, {"KM", CountryIsoCode::KM},
     {"CG", CountryIsoCode::CG}, {"CD", CountryIsoCode::CD},
     {"CK", CountryIsoCode::CK}, {"CR", CountryIsoCode::CR},
     {"CI", CountryIsoCode::CI}, {"HR", CountryIsoCode::HR},
     {"CU", CountryIsoCode::CU}, {"CW", CountryIsoCode::CW},
     {"CY", CountryIsoCode::CY}, {"CZ", CountryIsoCode::CZ},
     {"DK", CountryIsoCode::DK}, {"DJ", CountryIsoCode::DJ},
     {"DM", CountryIsoCode::DM}, {"DO", CountryIsoCode::DO},
     {"EC", CountryIsoCode::EC}, {"EG", CountryIsoCode::EG},
     {"SV", CountryIsoCode::SV}, {"GQ", CountryIsoCode::GQ},
     {"ER", CountryIsoCode::ER}, {"EE", CountryIsoCode::EE},
     {"ET", CountryIsoCode::ET}, {"FK", CountryIsoCode::FK},
     {"FO", CountryIsoCode::FO}, {"FJ", CountryIsoCode::FJ},
     {"FI", CountryIsoCode::FI}, {"FR", CountryIsoCode::FR},
     {"GF", CountryIsoCode::GF}, {"PF", CountryIsoCode::PF},
     {"TF", CountryIsoCode::TF}, {"GA", CountryIsoCode::GA},
     {"GM", CountryIsoCode::GM}, {"GE", CountryIsoCode::GE},
     {"DE", CountryIsoCode::DE}, {"GH", CountryIsoCode::GH},
     {"GI", CountryIsoCode::GI}, {"GR", CountryIsoCode::GR},
     {"GL", CountryIsoCode::GL}, {"GD", CountryIsoCode::GD},
     {"GP", CountryIsoCode::GP}, {"GU", CountryIsoCode::GU},
     {"GT", CountryIsoCode::GT}, {"GG", CountryIsoCode::GG},
     {"GN", CountryIsoCode::GN}, {"GW", CountryIsoCode::GW},
     {"GY", CountryIsoCode::GY}, {"HT", CountryIsoCode::HT},
     {"HM", CountryIsoCode::HM}, {"VA", CountryIsoCode::VA},
     {"HN", CountryIsoCode::HN}, {"HK", CountryIsoCode::HK},
     {"HU", CountryIsoCode::HU}, {"IS", CountryIsoCode::IS},
     {"IN", CountryIsoCode::IN}, {"ID", CountryIsoCode::ID},
     {"IR", CountryIsoCode::IR}, {"IQ", CountryIsoCode::IQ},
     {"IE", CountryIsoCode::IE}, {"IM", CountryIsoCode::IM},
     {"IL", CountryIsoCode::IL}, {"IT", CountryIsoCode::IT},
     {"JM", CountryIsoCode::JM}, {"JP", CountryIsoCode::JP},
     {"JE", CountryIsoCode::JE}, {"JO", CountryIsoCode::JO},
     {"KZ", CountryIsoCode::KZ}, {"KE", CountryIsoCode::KE},
     {"KI", CountryIsoCode::KI}, {"KP", CountryIsoCode::KP},
     {"KR", CountryIsoCode::KR}, {"KW", CountryIsoCode::KW},
     {"KG", CountryIsoCode::KG}, {"LA", CountryIsoCode::LA},
     {"LV", CountryIsoCode::LV}, {"LB", CountryIsoCode::LB},
     {"LS", CountryIsoCode::LS}, {"LR", CountryIsoCode::LR},
     {"LY", CountryIsoCode::LY}, {"LI", CountryIsoCode::LI},
     {"LT", CountryIsoCode::LT}, {"LU", CountryIsoCode::LU},
     {"MO", CountryIsoCode::MO}, {"MK", CountryIsoCode::MK},
     {"MG", CountryIsoCode::MG}, {"MW", CountryIsoCode::MW},
     {"MY", CountryIsoCode::MY}, {"MV", CountryIsoCode::MV},
     {"ML", CountryIsoCode::ML}, {"MT", CountryIsoCode::MT},
     {"MH", CountryIsoCode::MH}, {"MQ", CountryIsoCode::MQ},
     {"MR", CountryIsoCode::MR}, {"MU", CountryIsoCode::MU},
     {"YT", CountryIsoCode::YT}, {"MX", CountryIsoCode::MX},
     {"FM", CountryIsoCode::FM}, {"MD", CountryIsoCode::MD},
     {"MC", CountryIsoCode::MC}, {"MN", CountryIsoCode::MN},
     {"ME", CountryIsoCode::ME}, {"MS", CountryIsoCode::MS},
     {"MA", CountryIsoCode::MA}, {"MZ", CountryIsoCode::MZ},
     {"MM", CountryIsoCode::MM}, {"NA", CountryIsoCode::NA},
     {"NR", CountryIsoCode::NR}, {"NP", CountryIsoCode::NP},
     {"NL", CountryIsoCode::NL}, {"NC", CountryIsoCode::NC},
     {"NZ", CountryIsoCode::NZ}, {"NI", CountryIsoCode::NI},
     {"NE", CountryIsoCode::NE}, {"NG", CountryIsoCode::NG},
     {"NU", CountryIsoCode::NU}, {"NF", CountryIsoCode::NF},
     {"MP", CountryIsoCode::MP}, {"NO", CountryIsoCode::NO},
     {"OM", CountryIsoCode::OM}, {"PK", CountryIsoCode::PK},
     {"PW", CountryIsoCode::PW}, {"PS", CountryIsoCode::PS},
     {"PA", CountryIsoCode::PA}, {"PG", CountryIsoCode::PG},
     {"PY", CountryIsoCode::PY}, {"PE", CountryIsoCode::PE},
     {"PH", CountryIsoCode::PH}, {"PN", CountryIsoCode::PN},
     {"PL", CountryIsoCode::PL}, {"PT", CountryIsoCode::PT},
     {"PR", CountryIsoCode::PR}, {"QA", CountryIsoCode::QA},
     {"RE", CountryIsoCode::RE}, {"RO", CountryIsoCode::RO},
     {"RU", CountryIsoCode::RU}, {"RW", CountryIsoCode::RW},
     {"BL", CountryIsoCode::BL}, {"SH", CountryIsoCode::SH},
     {"KN", CountryIsoCode::KN}, {"LC", CountryIsoCode::LC},
     {"MF", CountryIsoCode::MF}, {"PM", CountryIsoCode::PM},
     {"VC", CountryIsoCode::VC}, {"WS", CountryIsoCode::WS},
     {"SM", CountryIsoCode::SM}, {"ST", CountryIsoCode::ST},
     {"SA", CountryIsoCode::SA}, {"SN", CountryIsoCode::SN},
     {"RS", CountryIsoCode::RS}, {"SC", CountryIsoCode::SC},
     {"SL", CountryIsoCode::SL}, {"SG", CountryIsoCode::SG},
     {"SX", CountryIsoCode::SX}, {"SK", CountryIsoCode::SK},
     {"SI", CountryIsoCode::SI}, {"SB", CountryIsoCode::SB},
     {"SO", CountryIsoCode::SO}, {"ZA", CountryIsoCode::ZA},
     {"GS", CountryIsoCode::GS}, {"SS", CountryIsoCode::SS},
     {"ES", CountryIsoCode::ES}, {"LK", CountryIsoCode::LK},
     {"SD", CountryIsoCode::SD}, {"SR", CountryIsoCode::SR},
     {"SJ", CountryIsoCode::SJ}, {"SZ", CountryIsoCode::SZ},
     {"SE", CountryIsoCode::SE}, {"CH", CountryIsoCode::CH},
     {"SY", CountryIsoCode::SY}, {"TW", CountryIsoCode::TW},
     {"TJ", CountryIsoCode::TJ}, {"TZ", CountryIsoCode::TZ},
     {"TH", CountryIsoCode::TH}, {"TL", CountryIsoCode::TL},
     {"TG", CountryIsoCode::TG}, {"TK", CountryIsoCode::TK},
     {"TO", CountryIsoCode::TO}, {"TT", CountryIsoCode::TT},
     {"TN", CountryIsoCode::TN}, {"TR", CountryIsoCode::TR},
     {"TM", CountryIsoCode::TM}, {"TC", CountryIsoCode::TC},
     {"TV", CountryIsoCode::TV}, {"UG", CountryIsoCode::UG},
     {"UA", CountryIsoCode::UA}, {"AE", CountryIsoCode::AE},
     {"GB", CountryIsoCode::GB}, {"US", CountryIsoCode::US},
     {"UM", CountryIsoCode::UM}, {"UY", CountryIsoCode::UY},
     {"UZ", CountryIsoCode::UZ}, {"VU", CountryIsoCode::VU},
     {"VE", CountryIsoCode::VE}, {"VN", CountryIsoCode::VN},
     {"VG", CountryIsoCode::VG}, {"VI", CountryIsoCode::VI},
     {"WF", CountryIsoCode::WF}, {"EH", CountryIsoCode::EH},
     {"YE", CountryIsoCode::YE}, {"ZM", CountryIsoCode::ZM},
     {"ZW", CountryIsoCode::ZW}, {"XX", CountryIsoCode::XX}};

CountryIsoCode CountryIsoCodeFromString(std::string code) {
  auto iter = country_code_map.find(code);
  if (iter != country_code_map.end()) {
    return iter->second;
  }
  return CountryIsoCode::XX;
}
}  // namespace utils
}  // namespace nekit
