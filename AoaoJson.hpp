#ifndef AOAOJSON_HPP
#define AOAOJSON_HPP

#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <unordered_map>
#include <fstream>
#include <memory>

namespace aoao{

namespace json{

    //these charactors are treated as white space
    const std::array<char,4> ws_chars{
        0x20,//space
        0x09,//h tab
        0x0A,//LF
        0x0D//CR
    };

    bool is_ws(char c){
        for(const auto& ws_char : ws_chars){
            if(c == ws_char) return true;
        }
        return false;
    };

    //Value class family
    class Value{
        public:
            virtual std::string to_str(std::size_t indent_level=1) const =0;
            virtual ~Value()=default;
    };

    class Value_false : public Value{
        public:
            std::string to_str(std::size_t indent_level=1) const noexcept override;
    };
    std::string Value_false::to_str([[maybe_unused]]std::size_t indent_level) const noexcept{return "false";}

    class Value_true : public Value{
        public:
            std::string to_str(std::size_t indent_level=1) const noexcept override;
    };
    std::string Value_true::to_str([[maybe_unused]]std::size_t indent_level) const noexcept{return "true";}

    class Value_null : public Value{
        public:
            std::string to_str(std::size_t indent_level=1) const noexcept override;
    };
    std::string Value_null::to_str([[maybe_unused]]std::size_t indent_level) const noexcept{return "null";}

    class Value_string : public Value{
        public:
            Value_string(const std::string& str)
                :str(str){};
            std::string to_str(std::size_t indent_level=1) const noexcept override;
        private:
            std::string str;
    };
    std::string Value_string::to_str([[maybe_unused]]std::size_t indent_level) const noexcept{return "\""+str+"\"";}

    std::size_t find_string(const std::string& str){
    //std::string find_string(const std::string& str) noexcept try{
        if(str.front() != '"') throw std::runtime_error("illegal string format");
        //search end quotation
        auto end_pos = 0u;
        for(auto pos=1u;pos<str.size();pos++){
            if(str[pos] == '"'){
                end_pos = pos;
                break;
            }else if(str[pos] == 0x5C){//escaped
                if(pos == str.size()) break;
                pos++;
                if(
                    str[pos] == 0x22 or//quotation(")
                    str[pos] == 0x5C or//back slash(\)
                    str[pos] == 0x2F or//slash (/)
                    str[pos] == 0x62//back space(b)
                )continue;
                else throw std::runtime_error("illegal string format : illegal escaped charactor");
            }
        }
        if(end_pos == 0) throw std::runtime_error("illegal string format : cannot find end quotation");

        return end_pos+1;
    }

    class Value_object : public Value{
        public:
            std::string to_str(std::size_t indent_level=1) const noexcept override;
            std::unordered_map<std::string,std::unique_ptr<Value>> members;
    };
    std::string Value_object::to_str(std::size_t indent_level) const noexcept{
        auto ret = std::string();
        ret += "{\n";
        for(const auto& v : members){
            for(auto i=0u;i<indent_level;i++){
                ret += "  ";
            }
            ret += '"';
            ret += v.first;
            ret += '"';
            ret += " : ";
            ret += v.second->to_str(indent_level+1);
            ret += ',';
            ret += "\n";
        }
        //delete last comma
        if(ret.size() > 1) ret.pop_back();
        if(ret.size() > 1) ret.pop_back();
        if(ret.size() > 1) ret+='\n';
        for(auto i=0u;i<indent_level-1;i++){
                ret += "  ";
        }
        ret+="}";

        return ret;
    }

    //foward decl.
    std::size_t find_value(const std::string&);
    std::unique_ptr<json::Value> make_value(const std::string&) noexcept;

    std::size_t find_object(const std::string& str){
        if(str.front() != '{') throw std::runtime_error("cannot find object begin @ find_object");

        auto current_pos = 1u;

        while(true){
            //find end object
            if(is_ws(str[current_pos]) == true){
                current_pos++;
                continue;
            }else if(str[current_pos] == '}'){
                break;
            }
            //find key
            for(;;current_pos++){
                if(str[current_pos] == '"'){
                    const auto len = find_string(str.substr(current_pos));
                    current_pos = current_pos + len;
                    break;
                }
                else if(is_ws(str[current_pos]) == true) continue;
                else throw std::runtime_error("cannot find key @ find_object : "+str.substr(current_pos));
            }
            //find separator
            for(;;current_pos++){
                if(str[current_pos] == ':'){
                    current_pos++;
                    break;
                }
                else if(is_ws(str[current_pos]) == true) continue;
                else throw std::runtime_error("cannot find separator");
            }
            //find value
            for(;;current_pos++){
                if(is_ws(str[current_pos]) == true) continue;
                const auto len = find_value(str.substr(current_pos));
                current_pos += len;
                break;
                //else throw std::runtime_error("cannot find value");
            }
            //find value separator
            for(;;current_pos++){
                if(is_ws(str[current_pos]) == true) continue;
                else if(str[current_pos] == ','){
                    current_pos++;
                    break;
                }else{
                    break;
                }
            }
        }

        return current_pos+1;
    }

    Value_object make_object(const std::string& str){
        if(str.front() != '{') throw std::runtime_error("cannot find object begin @ make_object");
        if(str.back() != '}') throw std::runtime_error("cannot find object end @ make_object");

        auto ret = Value_object();
        auto current_pos = 1u;

        while(true){
            //find end object
            if(is_ws(str[current_pos]) == true){
                current_pos++;
                continue;
            }else if(str[current_pos] == '}'){
                break;
            }
            //find key
            std::string key;
            for(;;current_pos++){
                if(str[current_pos] == '"'){
                    const auto len = find_string(str.substr(current_pos));
                    key = str.substr(current_pos+1,len-2);
                    current_pos = current_pos + len;
                    break;
                }
                else if(is_ws(str[current_pos]) == true) continue;
                else throw std::runtime_error("cannot find key : "+str);
            }
            //find separator
            for(;;current_pos++){
                if(str[current_pos] == ':'){
                    current_pos++;
                    break;
                }
                else if(is_ws(str[current_pos]) == true) continue;
                else throw std::runtime_error("cannot find separator");
            }
            //find value
            for(;;current_pos++){
                if(is_ws(str[current_pos]) == true) continue;
                const auto len = find_value(str.substr(current_pos));
                ret.members[key] = make_value(str.substr(current_pos,len));
                current_pos += len;
                break;
                //else throw std::runtime_error("cannot find value");
            }
            //find value separator
            for(;;current_pos++){
                if(is_ws(str[current_pos]) == true) continue;
                else if(str[current_pos] == ','){
                    current_pos++;
                    break;
                }else{
                    break;
                }
            }
        }
        return ret;
    };

    class Value_array : public Value{
        public:
            std::string to_str(std::size_t indent_level=1) const noexcept override;
            std::vector<std::unique_ptr<Value>> values;
    };
    std::string Value_array::to_str([[maybe_unused]]std::size_t indent_level) const noexcept{
        std::string str;
        str += "[\n";
        
        for(const auto& v : values){
            for(auto i=0u;i<indent_level;++i){
                str += "  ";
            }
            str += v->to_str(indent_level+1);
            str += ",\n";
        }
        //remove comma
        if(str.size() > 1) str.pop_back();
        if(str.size() > 1) str.pop_back();
        if(str.size() > 1) str += '\n';
        for(auto i=0u;i<indent_level-1;++i){
                str += "  ";
        }
        str += ']';
        
        return str;
    }

    std::size_t find_array(const std::string& str){
        if(str.front() != '[') throw std::runtime_error("cannot find array begin @ find_array");

        auto current_pos = 1u;
        while(true){
            if(is_ws(str[current_pos]) == true){
                current_pos++;
                continue;
            }else if(str[current_pos] == ']'){
                break;
            }

            //find value
            for(;;current_pos++){
                if(is_ws(str[current_pos]) == true) continue;
                const auto len = find_value(str.substr(current_pos));
                current_pos += len;
                break;
            }

            //find array separator
            for(;;current_pos++){
                if(is_ws(str[current_pos]) == true) continue;
                else if(str[current_pos] == ','){
                    current_pos++;
                    break;
                }else break;
            }
        }

        return  current_pos+1;
    }

    Value_array make_array(const std::string& str){
        auto ret = Value_array();
        if(str.front() != '[') throw std::runtime_error("cannot find array begin @ make_array");
        if(str.back() != ']')  throw std::runtime_error("cannot find array begin @ make_array");

        auto current_pos = 1u;
        while(true){
            if(is_ws(str[current_pos]) == true){
                current_pos++;
                continue;
            }else if(str[current_pos] == ']'){
                break;
            }

            //find value
            for(;;current_pos++){
                if(is_ws(str[current_pos]) == true) continue;
                const auto len = find_value(str.substr(current_pos));
                ret.values.push_back(make_value(str.substr(current_pos,len)));
                current_pos += len;
                break;
            }

            //find array separator
            for(;;current_pos++){
                if(is_ws(str[current_pos]) == true) continue;
                else if(str[current_pos] == ','){
                    current_pos++;
                    break;
                }else break;
            }
        }

        return  ret;
    }

    class Value_int : public Value{
        public:
            std::string to_str(std::size_t indent_level=1) const noexcept override;
            int n;
    };

    std::string Value_int::to_str(std::size_t indent_level) const noexcept{
        return std::to_string(n);
    };

    class Value_double : public Value{
        public:
            std::string to_str(std::size_t indent_level=1) const noexcept override;
            double d;
    };

    bool is_num(const char& c){
        if(c == '-' or c== '0' or c=='1' or
            c == '2' or c== '3' or c=='4' or
            c == '5' or c== '6' or c=='7' or
            c == '8' or c== '9') return true;
        else return false;
    }

    bool is_int(const std::string& str){
        auto current_pos = 0u;
        if(str.front() == '-') current_pos++;

        auto zero_flag = false;
        if(str[current_pos]== '0'){
            zero_flag = true;
            current_pos++;
        }

        for(;current_pos<str.size();current_pos++){
            if(str[current_pos] >= 0x30 and str[current_pos] <= 0x39){
                if(zero_flag == false) continue;
                else throw std::runtime_error("illegal number sequence : "+str);
            }else if(str[current_pos] == '.') return false;
            else if(str[current_pos] == 'e' or str[current_pos] == 'E') return false;
            else throw std::runtime_error("illegal number sequence : "+str);
        }

        return true;
    }

    std::size_t find_int(const std::string& str){
        auto current_pos = 0u;

        if(str.front() == '-') current_pos++;
        if(str[current_pos] == '0') return 1;

        for(;current_pos<str.size();current_pos++){
            if(str[current_pos] >= 0x30 and str[current_pos] <= 0x39){
                continue;
            }else{
                break;
            }
        }
        return current_pos;
    }

    std::size_t find_num(const std::string& str){
        return find_int(str);
    }

    Value_int make_int(const std::string& str){
        auto ret = Value_int();
        ret.n = std::stol(str);
        return ret;
    }

    std::size_t find_value(const std::string& str){
        if(str.substr(0,5) == "false") return 5;
        else if(str.substr(0,4) == "true") return 4;
        else if(str.substr(0,4) == "null") return 4;
        else if(str.front() == '"') return find_string(str);
        else if(str.front() == '{') return find_object(str);
        else if(str.front() == '[') return find_array(str);
        else if(is_num(str.front()) == true) return find_num(str);
        else throw std::runtime_error("cannot find value : "+str);
    }

    std::unique_ptr<json::Value> make_value(const std::string& str) noexcept try{
        if(str == "false"){
            return std::make_unique<json::Value_false>();
        }else if(str == "true"){
            return std::make_unique<json::Value_true>();
        }else if(str == "null"){
            return std::make_unique<json::Value_null>();
        }else if(str.front() == '"'){
            if(str.back() != '"') throw std::runtime_error("illegal string @ make_value");
            return std::make_unique<json::Value_string>(str.substr(1,str.size()-2));
        }else if(str.front() == '{'){
            return std::make_unique<json::Value_object>(make_object(str));
        }else if(str.front() == '['){
            return std::make_unique<json::Value_array>(make_array(str));
        }else if(is_num(str.front()) == true){
            return std::make_unique<json::Value_int>(make_int(str));
        }else{
            throw std::runtime_error("illegal JSON value");
        }
    } catch(std::exception& e){
        std::cerr << e.what() << std::endl;
        return nullptr;
    }


}

class Json{
    public:
        //default ctor
        Json() = default;
        //ctor from filename
        Json(const std::string& filename);
        //copy ctor
        Json(const Json&);

        std::string to_str() const;

    private:
        std::unique_ptr<json::Value> p_value = nullptr;
        
};

//ctor from filename
Json::Json(const std::string& filename){
    //open file
    std::ifstream ifs(filename);
    if(ifs.fail()) throw std::runtime_error("error @ open JSON file. file_name is "+std::string(filename));

    //raw string
    std::string raw_str;
    {
        char buf;
        while(ifs.get(buf),ifs.eof() != true){
            raw_str += buf;
        }
    }

    //remove white space
    std::string value_str;
    {
        auto head_pos = 0u;
        for(;head_pos < raw_str.size();head_pos++){
            if(json::is_ws(raw_str[head_pos])) continue;
            break;
        }
        auto tail_pos = raw_str.size()-1;
        for(;tail_pos != 0;tail_pos--){
            if(json::is_ws(raw_str[tail_pos])) continue;
            break;
        }
        value_str = raw_str.substr(head_pos,tail_pos-head_pos+1);
    }


    //get value from raw string
    p_value = json::make_value(value_str);

}

//copy ctor
Json::Json(const Json& json){
    
};

std::string Json::to_str() const{
    return (p_value == nullptr) ? std::string() : p_value->to_str();
}


} // namespace aoao


#endif