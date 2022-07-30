#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <tuple>

#include "absl/strings/string_view.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/strip.h"    // StripPrefix(str, prefix), StripSuffix(str, suffix)
#include "absl/strings/str_split.h"  // StrSplit()
#include "absl/strings/str_join.h"
#include "absl/strings/str_format.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"


namespace StrUtils {
  // absl::StartsWith(str, prefix)
  static inline bool startswith(const std::string &str, const std::string &prefix) {
    return str.size() >= prefix.size() ? str.compare(0, prefix.size(), prefix) == 0 : false;
  }

  static inline bool startswith(const std::string &str, const char *prefix) {
    int prefix_len = strlen(prefix);
    return str.size() >= prefix_len ? str.compare(0, prefix_len, prefix) == 0 : false;
  }

  // absl::EndsWith(str, suffix)
  static inline bool endswith(const std::string &str, const std::string &surfix) {
    return str.size() >= surfix.size() ? str.compare(str.size() - surfix.size(), str.size(), surfix) == 0 : false;
  }

  static inline bool endswith(const std::string &str, const char *surfix) {
    int surfix_len = strlen(surfix);
    return str.size() >= surfix_len ? str.compare(str.size() - surfix_len, str.size(), surfix) == 0 : false;
  }


  // absl::StrContains();
  static inline bool contains(const std::string &source, const std::string &target, size_t pos = 0) {
    return source.find(target, pos) != std::string::npos;
  }

  static inline bool contains(const std::string &source, const char *target, size_t pos = 0) {
    return source.find(target, pos) != std::string::npos;
  }
};


TEST(STRING, Match) {
  std::string s = "tititoto";
  std::string prefix = "titi";
  std::string surfix = "toto";
  std::string sub = "tito";

  if (StrUtils::startswith(s, prefix.c_str())) {
    std::cout<< "startswith OK!" << std::endl;
  }

  if (StrUtils::endswith(s, surfix.c_str())) {
    std::cout<< "endswith OK!" << std::endl;
  }

  if (StrUtils::contains(s, sub)) {
    std::cout<< "contains OK!" << std::endl;
  }

}


TEST(STRING, SplitJoin) {
  /** absl::StrSplit 常用于解析集合数据
   * 1) absl::StrSplit的返回值可以直接转换成几乎所有的集合类型, 值的类型为std::string, 不能是std::string_view. 如下:
   *    - 序列: `std::vector`, `std::list`,`std::deque`, 
   *    - 集合: `std::set`, std::unordered_set`, `std::multiset`
   *    - 映射: `std::map`, `std::multimap`
   * 2) 关于split分割符, 可以有如下几种选择:
   *   - absl::ByChar, 
   *   - absl::ByString, 
   *   - absl::ByAnyChar, 
   *   - absl::ByLength, 
   *   - absl::MaxSplits, 
   */

  std::vector<std::string> split_res_vec = absl::StrSplit("b,a,c,a,b", ',');
  std::set<std::string> split_res_set = absl::StrSplit("b,a,c,a,b", ',');
  std::unordered_set<std::string> split_res_uset = absl::StrSplit("b,a,c,a,b", ',');
  std::pair<std::string, std::string> split_res_pair = absl::StrSplit("a,b,c", ',');

  // std::vector<std::string_view> split_res_vec2 = absl::StrSplit("b,a,c,a,b", ',');
  // std::set<const std::string_view> split_res_set2 = absl::StrSplit("b,a,c,a,b", ',');
  // std::unordered_set<const std::string_view> split_res_uset2 = absl::StrSplit("b,a,c,a,b", ',');

  // absl::StrSplit返回值可用于基于范围的for循环
  std::vector<std::string> split_res_vec3;
  for (const auto sv : absl::StrSplit("a,b,c", ',')) {
    if (sv != "b") split_res_vec3.emplace_back(sv);
  }

  std::map<std::string, std::string> split_res_map;
  for (absl::string_view sp : absl::StrSplit("a=b=c,d=e,f=,g", ',')) {
    // 同于absl::StrSplit返回什能自动适配容器, 所以这里OK
    split_res_map.insert(absl::StrSplit(sp, absl::MaxSplits('=', 1)));
  }
  EXPECT_EQ("b=c", split_res_map.find("a")->second);
  EXPECT_EQ("e", split_res_map.find("d")->second);
  EXPECT_EQ("", split_res_map.find("f")->second);
  EXPECT_EQ("", split_res_map.find("g")->second);

  /** absl::StrJoin 是容器输出的好工具:
   * 1) 对于std::vector/std::initializer_list这类同种类型的序列, 可以支持的元素类型有:
   *    string, int, float, double, bool等, 及它们的unique_ptr指针 (shared_ptr不行)
   * 
   * 2) 对于std::tuple这类不同种类型的序列也可以支持, 先用absl::AlphaNum转务string, 再输出
   * 
   * 3) 对于std::set这类无序序列, 也可以直接join, 但顺序不定
   * 
   * 4) 对于std::map/std::vector<std::pair<>> 这类关系型数据, 也可以直接join
   */

  std::vector<std::string> vs = {"foo", "bar", "baz"};
  std::string s1 = absl::StrJoin(vs, "-");
  EXPECT_EQ("foo-bar-baz", s1);

  std::string s2 = absl::StrJoin({"foo", "bar", "baz"}, "-");
  EXPECT_EQ("foo-bar-baz", s2);

  std::vector<int> vi = {1, 2, 3, -4};
  std::string s3 = absl::StrJoin(vi, "-");
  EXPECT_EQ("1-2-3--4", s3);

  int x = 1, y = 2, z = 3;
  std::vector<int*> vip = {&x, &y, &z};
  std::string s4 = absl::StrJoin(vip, "-");
  EXPECT_EQ("1-2-3", s4);

  std::vector<std::unique_ptr<int>> vp;
  vp.emplace_back(new int(1));
  vp.emplace_back(new int(2));
  vp.emplace_back(new int(3));
  std::string s5 = absl::StrJoin(vp, "-");
  EXPECT_EQ("1-2-3", s5);

  std::string s6 = absl::StrJoin(std::make_tuple(123, "abc", 0.456), "-");
  EXPECT_EQ("123-abc-0.456", s6);

  std::set<std::string> set = {"hi", "i", "am", "fitz"};
  std::string s7 = absl::StrJoin(set, ",");
  std::cout << s7 << std::endl;

  std::map<std::string, int> m = {
      std::make_pair("a", 1),
      std::make_pair("b", 2),
      std::make_pair("c", 3)};
  std::string s8 = absl::StrJoin(m, ",", absl::PairFormatter("="));
  EXPECT_EQ("a=1,b=2,c=3", s8);

}