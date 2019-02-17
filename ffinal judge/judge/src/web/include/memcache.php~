<?php
        
    require_once(dirname(__FILE__)."/db_info.inc.php");
    global $memcache;
    if ($OJ_MEMCACHE){
	$memcache = new Memcache;
	if($OJ_SAE)
				$memcache=memcache_init();
		else{
				$memcache->connect($OJ_MEMSERVER,  $OJ_MEMPORT);
	}
    }
    //if  use memcache,use set/get to save data ,if not return false
    # Gets key / value pair into memcache … called by mysql_query_cache()
    function getCache($key) {
        global $memcache;
        return ($memcache) ? $memcache->get($key) : false;
    }

    # Puts key / value pair into memcache … called by mysql_query_cache()
    function setCache($key, $object, $timeout = 60) {
        global $memcache;
        return ($memcache) ? $memcache->set($key,$object,MEMCACHE_COMPRESSED,$timeout) : false;
    }

    # Caching version of pdo_query()
    function mysql_query_cache($sql, $linkIdentifier = false,$timeout = 4) {
	global $OJ_NAME,$OJ_MEMCACHE;	

       //get data from memcached,if false then get data from database
       //use md5+sql as a key
        if (!($cache = getCache(md5($OJ_NAME.$_SERVER['HTTP_HOST']."mysql_query" . $sql)))) {

            $cache = false;
            $cache =pdo_query($sql);

         //if open the memcache,sava data 
                if (!setCache(md5($OJ_NAME.$_SERVER['HTTP_HOST']."mysql_query" . $sql), $cache, $timeout)) {
		 if($OJ_MEMCACHE) echo "You can run these command to get faster speed:<br>sudo apt-get install memcached<br>sudo apt-get install php5-memcache<br>sudo apt-get install php-memcache";
                }
        }	
        return $cache;
    }
?>
