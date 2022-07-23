#pragma GCC target("avx2")
#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")
 
#include <bits/stdc++.h>
using namespace std;
using ll=long long;
using ld=long double;
using pll=pair<ll, ll>;
using tll = tuple<ll, ll, ll>;
#define rep(i,n) for (ll i=0; i<n; ++i)
#define all(c) begin(c),end(c)
#define rall(c) rbegin(c),rend(c)
#define PI acos(-1)
#define oo 2e18
constexpr const ll MOD = 998244353;
void cincout(){ cin.tie(0); ios::sync_with_stdio(0); cout << fixed << setprecision(10); }
template<typename T1, typename T2>
bool chmax(T1 &a,T2 b){if(a<b){a=b;return true;}else return false;}
template<typename T1, typename T2>
bool chmin(T1 &a,T2 b){if(a>b){a=b;return true;}else return false;}
ld dist(ld x1,ld x2,ld y1,ld y2){return (sqrtl((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1)));}
// https://atcoder.jp/contests/abc228/editorial/2932
// M^(K^N)%MODを求める場合、 K^N%(MOD-1)
ll modpow(ll a, ll n, ll mod){ a %= mod; if (a==0) return 0; if (n==0) return 1; if (n%2==1) return (a * modpow(a, n - 1, mod)) % mod; else { ll t = modpow(a, n / 2, mod) % mod;       return (t * t) % mod; }}
ll modinv(ll n, ll mod){ return modpow(n, mod-2, mod); }
// 逆元（modが素数以外）https://atcoder.jp/contests/abc186/tasks/abc186_e
ll modinv2(ll a, ll m) { // ret = inv(a) mod m
  ll b=m, u=1, v=0; while (b){ ll t=a/b; a-=t*b; swap(a, b); u-=t*v; swap(u, v); } u%=m; if (u<0) u+=m; return u; }
// 座標圧縮
template<typename T>
ll comp(vector<T> &A){ map<T, ll> mp; for(auto p: A) mp[p] = 0; ll sz = 0; for(auto &p: mp) p.second = ++sz; for(auto &a: A) a = mp[a]; return sz; }
ll dx[] = {0, 1, 0, -1};
ll dy[] = {-1, 0, 1, 0};  
bool is_pop(ll hash, ll d){ return (hash>>d)&1; }
bool isn_field(ll y, ll x, ll H, ll W){
  return (y < 0 || y >= H || x < 0 || x >= W);
}
//priority_queue<ll, vector<ll>, greater<ll>> Q;

//    size_t = 18446744073709551615 (1.8*10^19)
// LLONG_MAX = 9223372036854775807 (9.2*10^18)
//   INT_MAX = 2147483647 (2.1*10^9)

/*
if (szAAA) std::cerr << "szAAA    :" << szAAA << std::endl;
*/

int main(){
  cincout();
  
  string S;
  while (1){
    cin >> S;
    cout << "if (" << S << ") ";
    cout << "std::cerr << \"";
    cout << left << setw(20) << S << ":\" << ";
    cout << S << " << std::endl;";
    cout << endl;
    if (cin.eof()) break;
  }
  return 0;
}