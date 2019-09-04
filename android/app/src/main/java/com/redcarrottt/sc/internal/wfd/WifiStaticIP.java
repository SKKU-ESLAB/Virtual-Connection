package com.redcarrottt.sc.internal.wfd;

import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.InetAddress;
import java.util.ArrayList;

/* Copyright (c) 2019, contributors. All rights reserved.
 *
 * Contributor: Gyeonghwan Hong <redcarrottt@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
@SuppressWarnings({"JavaReflectionMemberAccess", "unchecked", "RedundantArrayCreation"})
public class WifiStaticIP {
    @SuppressWarnings("unchecked")
    public static void setStaticIpConfiguration(WifiManager manager, WifiConfiguration config,
                                                InetAddress ipAddress, int prefixLength,
                                                InetAddress gateway, InetAddress[] dns) throws ClassNotFoundException, IllegalAccessException, IllegalArgumentException, InvocationTargetException, NoSuchMethodException, NoSuchFieldException, InstantiationException {
        // First set up IpAssignment to STATIC.
        Object ipAssignment = getEnumValue("android.net.IpConfiguration$IpAssignment", "STATIC");
        callMethod(config, "setIpAssignment", new String[]{"android.net" +
                ".IpConfiguration$IpAssignment"}, new Object[]{ipAssignment});

        // Then set properties in StaticIpConfiguration.
        Object staticIpConfig = newInstance("android.net.StaticIpConfiguration");
        Object linkAddress = newInstance("android.net.LinkAddress",
                new Class<?>[]{InetAddress.class, int.class}, new Object[]{ipAddress,
                        prefixLength});

        setField(staticIpConfig, "ipAddress", linkAddress);
        setField(staticIpConfig, "gateway", gateway);
        getField(staticIpConfig, "dnsServers", ArrayList.class).clear();
        for (int i = 0; i < dns.length; i++)
            getField(staticIpConfig, "dnsServers", ArrayList.class).add(dns[i]);

        callMethod(config, "setStaticIpConfiguration", new String[]{"android.net" +
                ".StaticIpConfiguration"}, new Object[]{staticIpConfig});
        manager.updateNetwork(config);
        manager.saveConfiguration();
    }

    private static Object newInstance(String className) throws ClassNotFoundException,
            InstantiationException, IllegalAccessException, NoSuchMethodException,
            IllegalArgumentException, InvocationTargetException {
        return newInstance(className, new Class<?>[0], new Object[0]);
    }

    private static Object newInstance(String className, Class<?>[] parameterClasses,
                                      Object[] parameterValues) throws NoSuchMethodException,
            InstantiationException, IllegalAccessException, IllegalArgumentException,
            InvocationTargetException, ClassNotFoundException {
        Class<?> clz = Class.forName(className);
        Constructor<?> constructor = clz.getConstructor(parameterClasses);
        return constructor.newInstance(parameterValues);
    }

    @SuppressWarnings({"unchecked", "rawtypes"})
    private static Object getEnumValue(String enumClassName, String enumValue) throws ClassNotFoundException {
        Class<Enum> enumClz = (Class<Enum>) Class.forName(enumClassName);
        return Enum.valueOf(enumClz, enumValue);
    }

    private static void setField(Object object, String fieldName, Object value) throws IllegalAccessException, IllegalArgumentException, NoSuchFieldException {
        Field field = object.getClass().getDeclaredField(fieldName);
        field.set(object, value);
    }

    private static <T> T getField(Object object, String fieldName, Class<T> type) throws IllegalAccessException, IllegalArgumentException, NoSuchFieldException {
        Field field = object.getClass().getDeclaredField(fieldName);
        return type.cast(field.get(object));
    }

    private static void callMethod(Object object, String methodName, String[] parameterTypes,
                                   Object[] parameterValues) throws ClassNotFoundException,
            IllegalAccessException, IllegalArgumentException, InvocationTargetException,
            NoSuchMethodException {
        Class<?>[] parameterClasses = new Class<?>[parameterTypes.length];
        for (int i = 0; i < parameterTypes.length; i++)
            parameterClasses[i] = Class.forName(parameterTypes[i]);

        Method method = object.getClass().getDeclaredMethod(methodName, parameterClasses);
        method.invoke(object, parameterValues);
    }

//    public static void setIpAssignment(String assign, WifiConfiguration wifiConf) throws
// SecurityException, IllegalArgumentException, NoSuchFieldException, IllegalAccessException {
//        setEnumField(wifiConf, assign, "ipAssignment");
//    }
//
//    public static void setIpAddress(InetAddress addr, int prefixLength,
//                                    WifiConfiguration wifiConf) throws SecurityException,
//            IllegalArgumentException, NoSuchFieldException, IllegalAccessException,
//            NoSuchMethodException, ClassNotFoundException, InstantiationException,
//            InvocationTargetException {
//        Object linkProperties = getField(wifiConf, "linkProperties");
//        if (linkProperties == null) return;
//        Class laClass = Class.forName("android.net.LinkAddress");
//        Constructor laConstructor = laClass.getConstructor(new Class[]{InetAddress.class,
//                int.class});
//        Object linkAddress = laConstructor.newInstance(addr, prefixLength);
//
//        ArrayList mLinkAddresses = (ArrayList) getDeclaredField(linkProperties, "mLinkAddresses");
//        mLinkAddresses.clear();
//        mLinkAddresses.add(linkAddress);
//    }
//
//    public static void setGateway(InetAddress gateway, WifiConfiguration wifiConf) throws
// SecurityException, IllegalArgumentException, NoSuchFieldException, IllegalAccessException,
// ClassNotFoundException, NoSuchMethodException, InstantiationException,
// InvocationTargetException {
//        Object linkProperties = getField(wifiConf, "linkProperties");
//        if (linkProperties == null) return;
//        Class routeInfoClass = Class.forName("android.net.RouteInfo");
//        Constructor routeInfoConstructor =
//                routeInfoClass.getConstructor(new Class[]{InetAddress.class});
//        Object routeInfo = routeInfoConstructor.newInstance(gateway);
//
//        ArrayList mRoutes = (ArrayList) getDeclaredField(linkProperties, "mRoutes");
//        mRoutes.clear();
//        mRoutes.add(routeInfo);
//    }
//
//    public static void setDNS(InetAddress dns, WifiConfiguration wifiConf) throws
// SecurityException, IllegalArgumentException, NoSuchFieldException, IllegalAccessException {
//        Object linkProperties = getField(wifiConf, "linkProperties");
//        if (linkProperties == null) return;
//
//        ArrayList<InetAddress> mDnses = (ArrayList<InetAddress>) getDeclaredField(linkProperties,
//                "mDnses");
//        mDnses.clear(); //or add a new dns address , here I just want to replace DNS1
//        mDnses.add(dns);
//    }
//
//    public static Object getField(Object obj, String name) throws SecurityException,
//            NoSuchFieldException, IllegalArgumentException, IllegalAccessException {
//        Field f = obj.getClass().getField(name);
//        Object out = f.get(obj);
//        return out;
//    }
//
//    public static Object getDeclaredField(Object obj, String name) throws SecurityException,
//            NoSuchFieldException, IllegalArgumentException, IllegalAccessException {
//        Field f = obj.getClass().getDeclaredField(name);
//        f.setAccessible(true);
//        Object out = f.get(obj);
//        return out;
//    }
//
//    private static void setEnumField(Object obj, String value, String name) throws
// SecurityException, NoSuchFieldException, IllegalArgumentException, IllegalAccessException {
//        Field f = obj.getClass().getField(name);
//        f.set(obj, Enum.valueOf((Class<Enum>) f.getType(), value));
//    }
}
